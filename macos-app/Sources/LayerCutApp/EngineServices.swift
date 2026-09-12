import Foundation

struct EngineDiagnostic: Sendable, Equatable {
    enum Severity: Sendable { case warning, error }
    let message: String
    let severity: Severity
}

struct MeshMetadata: Sendable, Equatable {
    let path: String
    let triangleCount: Int
    let volume: Double
    let bounds: (min: SIMD3<Float>, max: SIMD3<Float>)
    let diagnostics: [EngineDiagnostic]

    static func == (lhs: MeshMetadata, rhs: MeshMetadata) -> Bool {
        lhs.path == rhs.path && lhs.triangleCount == rhs.triangleCount &&
        lhs.volume == rhs.volume && lhs.bounds.min == rhs.bounds.min &&
        lhs.bounds.max == rhs.bounds.max && lhs.diagnostics == rhs.diagnostics
    }
}

struct MeshSnapshot: Sendable, Equatable {
    let vertices: [Float]
    let indices: [UInt32]
    let bounds: (min: SIMD3<Float>, max: SIMD3<Float>)

    static func == (lhs: MeshSnapshot, rhs: MeshSnapshot) -> Bool {
        lhs.vertices == rhs.vertices && lhs.indices == rhs.indices &&
        lhs.bounds.min == rhs.bounds.min && lhs.bounds.max == rhs.bounds.max
    }
}

struct LayerOutput: Sendable {
    let index: Int
    let z: Double
    let isEmpty: Bool
    let svg: String?
    let previewSVG: String?
    let png: Data?
}

struct CricutPageOutput: Sendable {
    let index: Int
    let cutSVG: String
    let guideSVG: String
    let combinedSVG: String
    let layerStart: Int
    let layerCount: Int
    let pathCount: Int
}

struct SliceOutput: Sendable {
    let layers: [LayerOutput]
    let pages: [CricutPageOutput]
    let stackedSTL: Data?
    let warnings: [String]
    let diagnostics: [EngineDiagnostic]
}

final class ProgressBox: @unchecked Sendable {
    let handler: (@Sendable (Double) -> Void)?
    init(handler: (@Sendable (Double) -> Void)?) { self.handler = handler }
}

final class CancellationBox: @unchecked Sendable {
    let raw: UnsafeMutableRawPointer
    init(raw: UnsafeMutableRawPointer) { self.raw = raw }
}

private func engineProgress(_ context: UnsafeMutableRawPointer?, _ layer: Int32,
                            _ total: Int32, _ fraction: CDouble) {
    guard let context else { return }
    Unmanaged<ProgressBox>.fromOpaque(context).takeUnretainedValue().handler?(Double(fraction))
}

final class MeshService: Sendable {
    func load(path: String) async throws -> MeshMetadata {
        try await Task.detached(priority: .userInitiated) {
            let mesh = try loadStl(path: path)
            var bounds = LayerCutBounds(min_x: 0, min_y: 0, min_z: 0, max_x: 0, max_y: 0, max_z: 0)
            guard layer_cut_mesh_bounds(mesh.raw, &bounds) != 0 else {
                throw EngineError.lastError(lastErrorMessage() ?? "Could not read mesh bounds")
            }
            return MeshMetadata(path: path,
                                triangleCount: Int(layer_cut_mesh_triangle_count(mesh.raw)),
                                volume: layer_cut_mesh_volume(mesh.raw),
                                bounds: (SIMD3(bounds.min_x, bounds.min_y, bounds.min_z),
                                         SIMD3(bounds.max_x, bounds.max_y, bounds.max_z)),
                                diagnostics: meshDiagnostics(mesh.raw))
        }.value
    }

    func snapshot(path: String, transform: TransformSession) async throws -> MeshSnapshot {
        try await Task.detached(priority: .userInitiated) {
            let mesh = try loadStl(path: path)
            var snapshot: UnsafeMutableRawPointer?
            let value = transform.validated()
            guard layer_cut_mesh_snapshot_euler(mesh.raw, axisValue(value.cuttingAxis), value.rotateX, value.rotateY, value.rotateZ, value.scale, &snapshot) != 0,
                  let snapshot else { throw EngineError.lastError(lastErrorMessage() ?? "Could not create mesh snapshot") }
                defer { layer_cut_free_snapshot(snapshot) }
            var vertexCount = 0
            var indexCount = 0
            guard let vertexPointer = layer_cut_snapshot_vertices(snapshot, &vertexCount),
                  let indexPointer = layer_cut_snapshot_indices(snapshot, &indexCount) else {
                throw EngineError.noData("Mesh snapshot is empty")
            }
            var bounds = LayerCutBounds(min_x: 0, min_y: 0, min_z: 0, max_x: 0, max_y: 0, max_z: 0)
            guard layer_cut_snapshot_bounds(snapshot, &bounds) != 0 else {
                throw EngineError.lastError(lastErrorMessage() ?? "Could not read mesh bounds")
            }
            return MeshSnapshot(
                vertices: Array(UnsafeBufferPointer(start: vertexPointer, count: vertexCount)),
                indices: Array(UnsafeBufferPointer(start: indexPointer, count: indexCount)),
                bounds: (SIMD3(bounds.min_x, bounds.min_y, bounds.min_z),
                         SIMD3(bounds.max_x, bounds.max_y, bounds.max_z)))
        }.value
    }

    func stackedSnapshot(path: String, profile: SlicingProfile, transform: TransformSession,
                         progress: (@Sendable (Double) -> Void)? = nil) async throws -> MeshSnapshot {
        // Stacked STL is supplemental preview data, not the export format.
        var stackedPreviewProfile = profile
        stackedPreviewProfile.outputFormat = "svg"
        let output = try await SlicingService().preview(path: path, profile: stackedPreviewProfile, transform: transform, progress: progress)
        guard let data = output.stackedSTL, !data.isEmpty else {
            throw EngineError.noData("This profile did not produce a stacked STL preview")
        }
        return try await snapshotFromStackedSTL(data)
    }

    /// Converts a stacked STL produced by a slice back into a viewport snapshot,
    /// so a slice can feed both the stacked 3D preview and the layer preview
    /// without slicing the model twice.
    func snapshotFromStackedSTL(_ data: Data) async throws -> MeshSnapshot {
        let temporaryURL = FileManager.default.temporaryDirectory
            .appendingPathComponent("layer-cut-stacked-\(UUID().uuidString).stl")
        try data.write(to: temporaryURL, options: .atomic)
        defer { try? FileManager.default.removeItem(at: temporaryURL) }
        // The stacked STL is generated from the already transformed slice. Reload
        // it without applying the session transform a second time.
        return try await snapshot(path: temporaryURL.path, transform: .identity)
    }
}

final class SlicingService: Sendable {
    func slice(path: String, profile: SlicingProfile, transform: TransformSession = .identity,
               includeLayerPreview: Bool = false,
               progress: (@Sendable (Double) -> Void)? = nil) async throws -> SliceOutput {
        let cancellation = try makeCancellation()
        return try await withTaskCancellationHandler {
            try await Task.detached(priority: .userInitiated) {
                defer { layer_cut_free_cancellation(cancellation.raw) }
                let mesh = try loadStl(path: path)
                let config = try createConfig()
                let progressBox = try configure(config, profile: profile, transform: transform, cancellation: cancellation.raw, includeLayerPreview: includeLayerPreview, progress: progress)
                _ = progressBox
                guard let resultRaw = layer_cut_slice(mesh.raw, config.raw) else {
                    throw EngineError.sliceFailed(lastErrorMessage() ?? "Unknown slice error")
                }
                let result = SliceResultHandle(raw: resultRaw)
                return collect(result)
            }.value
        } onCancel: {
            layer_cut_cancellation_cancel(cancellation.raw)
        }
    }

    func preview(path: String, profile: SlicingProfile, transform: TransformSession = .identity,
                 dpi: Int = 96,
                 includeLayerPreview: Bool = false,
                 progress: (@Sendable (Double) -> Void)? = nil) async throws -> SliceOutput {
        _ = dpi
        return try await slice(path: path, profile: profile, transform: transform, includeLayerPreview: includeLayerPreview, progress: progress)
    }

    private func makeCancellation() throws -> CancellationBox {
        guard let value = layer_cut_cancellation_create() else {
            throw EngineError.sliceFailed("Could not create cancellation token")
        }
        return CancellationBox(raw: value)
    }
}

final class ExportService: Sendable {
    private let slicing = SlicingService()

    func export(path: String, profile: SlicingProfile, transform: TransformSession = .identity,
                progress: (@Sendable (Double) -> Void)? = nil) async throws -> SliceOutput {
        try await slicing.slice(path: path, profile: profile, transform: transform, progress: progress)
    }
}

private func axisValue(_ axis: String) -> Int32 {
    ["+X", "-X", "+Y", "-Y", "+Z", "-Z"].firstIndex(of: axis).map(Int32.init) ?? 4
}

private func meshDiagnostics(_ mesh: UnsafeMutableRawPointer) -> [EngineDiagnostic] {
    (0..<Int(layer_cut_mesh_diagnostic_count(mesh))).compactMap { index in
        guard let value = layer_cut_mesh_diagnostic(mesh, Int32(index)) else { return nil }
        return EngineDiagnostic(message: String(cString: value),
                                severity: layer_cut_mesh_diagnostic_severity(mesh, Int32(index)) == 1 ? .error : .warning)
    }
}

private func configure(_ config: ConfigHandle, profile: SlicingProfile, transform: TransformSession,
                       cancellation: UnsafeMutableRawPointer,
                       includeLayerPreview: Bool,
                       progress: (@Sendable (Double) -> Void)?) throws -> ProgressBox {
    let value = transform.validated()
    guard layer_cut_config_set_transform_euler(config.raw, axisValue(value.cuttingAxis), value.rotateX, value.rotateY, value.rotateZ, value.scale) != 0 else { throw EngineError.sliceFailed(lastErrorMessage() ?? "Invalid transform") }
    try configSetLayerHeight(config, mm: profile.layerHeight)
    guard layer_cut_config_set_cancellation(config.raw, cancellation) != 0 else { throw EngineError.sliceFailed(lastErrorMessage() ?? "Could not configure cancellation") }
    try configSetFormat(config, outputFormat(profile.outputFormat))
    try configSetDpi(config, dpi: Int32(profile.outputDPI))
    try configSetCricutGap(config, gap: profile.cricutGap)
    guard layer_cut_config_set_cricut_packing(config.raw, profile.packing == "tight" ? 1 : 0) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Invalid Cricut packing strategy")
    }
    try configSetCricutGuideInset(config, inset: profile.guideInset)
    guard layer_cut_config_set_show_layer_numbers(config.raw, profile.showLayerNumbers ? 1 : 0) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Invalid layer numbering setting")
    }
    guard layer_cut_config_set_layer_number_font_size(config.raw, profile.layerNumberFontSize) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Invalid layer number font size")
    }
    try configSetCleanupMode(config, cleanupMode(profile.cleanupMode))
    try configSetCleanupThresholds(config, featureWidth: profile.featureWidth, islandArea: profile.islandArea, holeWidth: profile.holeWidth, bridgeWidth: profile.bridgeWidth)
    guard layer_cut_config_set_layer_preview(config.raw, includeLayerPreview ? 1 : 0) != 0 else { throw EngineError.sliceFailed(lastErrorMessage() ?? "Invalid layer preview setting") }
    let box = ProgressBox(handler: progress)
    let context = Unmanaged.passUnretained(box).toOpaque()
    guard layer_cut_config_set_progress_callback(config.raw, engineProgress, context) != 0 else { throw EngineError.sliceFailed(lastErrorMessage() ?? "Could not configure progress") }
    return box
}

private func outputFormat(_ value: String) -> OutputFormat {
    switch value { case "png": return .png; case "cricut-large": return .cricutLarge; case "cricut-normal": return .cricutNormal; default: return .svg }
}

private func cleanupMode(_ value: String) -> CleanupMode {
    switch value { case "preserve": return .preserve; case "apply": return .apply; default: return .warn }
}

private func collect(_ result: SliceResultHandle) -> SliceOutput {
    let layers = (0..<Int(resultLayerCount(result))).map { index in
        LayerOutput(index: index, z: resultLayerZ(result, index: Int32(index)),
                    isEmpty: resultLayerIsEmpty(result, index: Int32(index)),
                     svg: resultLayerSvg(result, index: Int32(index)),
                     previewSVG: resultLayerPreviewSvg(result, index: Int32(index)),
                    png: resultLayerPng(result, index: Int32(index)))
    }
    let pages = (0..<Int(resultPageCount(result))).compactMap { index -> CricutPageOutput? in
        guard let cut = resultPageCutSvg(result, page: Int32(index)), let guide = resultPageGuideSvg(result, page: Int32(index)), let combined = layer_cut_result_page_combined_svg(result.raw, Int32(index)) else { return nil }
        return CricutPageOutput(index: index, cutSVG: cut, guideSVG: guide, combinedSVG: String(cString: combined), layerStart: Int(resultPageLayerStart(result, page: Int32(index))), layerCount: Int(resultPageLayerCount(result, page: Int32(index))), pathCount: Int(resultPagePathCount(result, page: Int32(index))))
    }
    var size = 0
    let stacked = layer_cut_result_stacked_stl(result.raw, &size).map { Data(bytes: $0, count: size) }
    let warnings = (0..<Int(resultWarningCount(result))).compactMap { resultWarning(result, index: Int32($0)) }
    let diagnostics = (0..<Int(layer_cut_result_diagnostic_count(result.raw))).compactMap { index -> EngineDiagnostic? in
        guard let value = layer_cut_result_diagnostic(result.raw, Int32(index)) else { return nil }
        return EngineDiagnostic(message: String(cString: value), severity: layer_cut_result_diagnostic_severity(result.raw, Int32(index)) == 1 ? .error : .warning)
    }
    return SliceOutput(layers: layers, pages: pages, stackedSTL: stacked, warnings: warnings, diagnostics: diagnostics)
}
