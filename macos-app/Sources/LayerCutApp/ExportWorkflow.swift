import Foundation

enum ExportCompletion: Sendable, Equatable {
    case completed
    case partialFailure
    case cancelled
}

struct ExportFailure: Sendable, Equatable {
    let path: String
    let message: String
}

struct ExportSummary: Sendable, Equatable {
    let profileName: String
    let format: String
    let layerCount: Int
    let pageCount: Int
    let totalBytes: Int
    let warnings: [String]
}

struct ExportReport: Sendable {
    let summary: ExportSummary
    let written: [String]
    let failures: [ExportFailure]
    let completion: ExportCompletion
}

enum ExportError: LocalizedError {
    case unsupportedFormat(String)
    case noOutputDirectory
    case overwriteConfirmationRequired([String])
    case cancelled
    case directoryAccessDenied

    var errorDescription: String? {
        switch self {
        case .unsupportedFormat(let format): return "Unsupported export format: \(format)"
        case .noOutputDirectory: return "Choose an output directory before exporting."
        case .overwriteConfirmationRequired(let paths):
            return "Existing files require overwrite confirmation: \(paths.joined(separator: ", "))"
        case .cancelled: return "Export cancelled."
        case .directoryAccessDenied: return "The selected output directory is no longer accessible."
        }
    }
}

enum ExportFileNaming {
    static func layer(index: Int, extension fileExtension: String) -> String {
        "layer_\(padded(index)).\(fileExtension)"
    }

    static func cricutPrefix(page: CricutPageOutput) -> String {
        "page_\(padded(page.index))_layers_\(padded(page.layerStart))-\(padded(page.layerStart + page.layerCount - 1))"
    }

    static func padded(_ value: Int) -> String {
        value < 1000 ? String(format: "%03d", value) : String(value)
    }
}

struct SecurityScopedDirectory: Sendable {
    let url: URL

    func withAccess<T: Sendable>(_ operation: () throws -> T) throws -> T {
        guard url.startAccessingSecurityScopedResource() else {
            throw ExportError.directoryAccessDenied
        }
        defer { url.stopAccessingSecurityScopedResource() }
        return try operation()
    }
}

final class ExportCoordinator: Sendable {
    private let slicing = ExportService()

    func run(path: String, profile: SlicingProfile, directory: SecurityScopedDirectory,
             allowOverwrite: Bool = false,
             progress: (@Sendable (Double) -> Void)? = nil) async throws -> ExportReport {
        let output = try await slicing.export(path: path, profile: profile, progress: progress)
        let artifacts = try artifacts(for: output, profile: profile)
        let existing = artifacts.filter { FileManager.default.fileExists(atPath: directory.url.appendingPathComponent($0.path).path) }
        guard allowOverwrite || existing.isEmpty else {
            throw ExportError.overwriteConfirmationRequired(existing.map(\.path))
        }

        do {
            return try await Task.detached(priority: .userInitiated) {
                try Task.checkCancellation()
                return try directory.withAccess {
                try FileManager.default.createDirectory(at: directory.url, withIntermediateDirectories: true)
                var written: [String] = []
                var failures: [ExportFailure] = []
                for (index, artifact) in artifacts.enumerated() {
                    do {
                        try Task.checkCancellation()
                        try artifact.data.write(to: directory.url.appendingPathComponent(artifact.path), options: .atomic)
                        written.append(artifact.path)
                    } catch is CancellationError {
                        throw ExportError.cancelled
                    } catch {
                        failures.append(ExportFailure(path: artifact.path, message: error.localizedDescription))
                    }
                    progress?(Double(index + 1) / Double(max(artifacts.count, 1)))
                }
                let summary = ExportSummary(profileName: profile.name, format: profile.outputFormat,
                                            layerCount: output.layers.count, pageCount: output.pages.count,
                                            totalBytes: artifacts.reduce(0) { $0 + $1.data.count },
                                            warnings: output.warnings + output.diagnostics.map(\.message))
                return ExportReport(summary: summary, written: written, failures: failures,
                                    completion: failures.isEmpty ? .completed : .partialFailure)
                }
            }.value
        } catch is CancellationError {
            throw ExportError.cancelled
        }
    }

    private func artifacts(for output: SliceOutput, profile: SlicingProfile) throws -> [ExportArtifact] {
        switch profile.outputFormat {
        case "svg":
            return output.layers.compactMap { layer in layer.svg.map { ExportArtifact(path: ExportFileNaming.layer(index: layer.index, extension: "svg"), data: Data($0.utf8)) } }
        case "png":
            return output.layers.compactMap { layer in layer.png.map { ExportArtifact(path: ExportFileNaming.layer(index: layer.index, extension: "png"), data: $0) } }
        case "cricut-normal", "cricut-large":
            return output.pages.flatMap { page -> [ExportArtifact] in
                let prefix = ExportFileNaming.cricutPrefix(page: page)
                if profile.combinedCricut {
                    return [ExportArtifact(path: prefix + ".svg", data: Data(page.combinedSVG.utf8))]
                }
                return [ExportArtifact(path: prefix + ".cut.svg", data: Data(page.cutSVG.utf8)),
                        ExportArtifact(path: prefix + ".guide.svg", data: Data(page.guideSVG.utf8))]
            }
        case "stacked-stl":
            guard let data = output.stackedSTL else { return [] }
            return [ExportArtifact(path: "stacked_preview.stl", data: data)]
        case "cricut-png":
            throw ExportError.unsupportedFormat("Cricut PNG pages are intentionally unsupported")
        default:
            throw ExportError.unsupportedFormat(profile.outputFormat)
        }
    }
}

private struct ExportArtifact: Sendable {
    let path: String
    let data: Data
}
