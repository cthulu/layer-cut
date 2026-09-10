import Foundation
import Combine
import SwiftUI

struct SlicingProfile: Identifiable, Equatable, Sendable {
    let id: UUID
    var name: String
    var layerHeight: Double
    var outputFormat: String
    var outputDPI: Int
    var combinedCricut: Bool
    var showLayerNumbers: Bool
    var layerNumberFontSize: Double
    var cleanupMode: String
    var featureWidth: Double
    var islandArea: Double
    var holeWidth: Double
    var bridgeWidth: Double
    var packing: String
    var cricutGap: Double
    var guideInset: Double
    var outputOptions: [String: String]

    static let defaultProfile = SlicingProfile(
        id: UUID(), name: "Default",
        layerHeight: 1, outputFormat: "cricut-normal", outputDPI: 96,
         combinedCricut: false,
         showLayerNumbers: false,
         layerNumberFontSize: 2.5,
        cleanupMode: "warn", featureWidth: 0.4, islandArea: 1,
        holeWidth: 0.4, bridgeWidth: 0.4, packing: "fixed", cricutGap: 3,
        guideInset: 1, outputOptions: [:]
    )

    init(id: UUID = UUID(), name: String, layerHeight: Double = 1, outputFormat: String = "cricut-normal",
          outputDPI: Int = 96, combinedCricut: Bool = false, showLayerNumbers: Bool = false, layerNumberFontSize: Double = 2.5, cleanupMode: String = "warn", featureWidth: Double = 0.4,
         islandArea: Double = 1, holeWidth: Double = 0.4, bridgeWidth: Double = 0.4,
         packing: String = "fixed", cricutGap: Double = 3, guideInset: Double = 1,
         outputOptions: [String: String] = [:]) {
        self.id = id
        self.name = name
        self.layerHeight = layerHeight
        self.outputFormat = outputFormat
        self.outputDPI = outputDPI
        self.combinedCricut = combinedCricut
        self.showLayerNumbers = showLayerNumbers
        self.layerNumberFontSize = layerNumberFontSize
        self.cleanupMode = cleanupMode
        self.featureWidth = featureWidth
        self.islandArea = islandArea
        self.holeWidth = holeWidth
        self.bridgeWidth = bridgeWidth
        self.packing = packing
        self.cricutGap = cricutGap
        self.guideInset = guideInset
        self.outputOptions = outputOptions
    }

    func validated() throws -> SlicingProfile {
        guard !name.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else { throw ProfileError.invalid("Profile name is empty") }
        guard layerHeight.isFinite, layerHeight > 0, layerHeight <= 100 else { throw ProfileError.invalid("Invalid layer height") }
        guard outputDPI > 0, cleanupMode == "preserve" || cleanupMode == "warn" || cleanupMode == "apply" else { throw ProfileError.invalid("Invalid output or cleanup settings") }
        guard ["fixed", "tight"].contains(packing), cricutGap.isFinite, guideInset >= 0, layerNumberFontSize.isFinite, layerNumberFontSize > 0 else { throw ProfileError.invalid("Invalid Cricut settings") }
        return self
    }
}

enum ProfileError: LocalizedError {
    case invalid(String)
    case unreadable(String)
    case unwritable(String)

    var errorDescription: String? {
        switch self { case .invalid(let message), .unreadable(let message), .unwritable(let message): return message }
    }
}

struct ProfileStore {
    static let currentVersion = 1
    let fileURL: URL

    init(fileURL: URL? = nil) {
        if let fileURL { self.fileURL = fileURL; return }
        let base: URL
        if let xdg = ProcessInfo.processInfo.environment["XDG_CONFIG_HOME"], !xdg.isEmpty {
            base = URL(fileURLWithPath: xdg, isDirectory: true)
        } else {
            base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        }
        self.fileURL = base.appendingPathComponent("layer-cut", isDirectory: true).appendingPathComponent("profiles.yaml")
    }

    func load() -> [SlicingProfile] {
        (try? loadValidated()) ?? [.defaultProfile]
    }

    func loadValidated() throws -> [SlicingProfile] {
        guard FileManager.default.fileExists(atPath: fileURL.path) else { return [.defaultProfile] }
        let text: String
        do { text = try String(contentsOf: fileURL, encoding: .utf8) }
        catch { throw ProfileError.unreadable(error.localizedDescription) }
        return try parse(text)
    }

    func save(_ profiles: [SlicingProfile]) throws {
        var checked: [SlicingProfile] = []
        var names = Set<String>()
        for profile in profiles {
            let value = try profile.validated()
            guard names.insert(value.name).inserted else { throw ProfileError.invalid("Profile names must be unique") }
            checked.append(value)
        }
        guard !checked.isEmpty else { throw ProfileError.invalid("At least one profile is required") }
        let directory = fileURL.deletingLastPathComponent()
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let temporary = directory.appendingPathComponent(".profiles-\(UUID().uuidString).tmp")
        do {
            try yaml(checked).write(to: temporary, atomically: false, encoding: .utf8)
            if FileManager.default.fileExists(atPath: fileURL.path) {
                _ = try FileManager.default.replaceItemAt(fileURL, withItemAt: temporary, backupItemName: nil, options: .usingNewMetadataOnly)
            } else {
                try FileManager.default.moveItem(at: temporary, to: fileURL)
            }
        } catch {
            try? FileManager.default.removeItem(at: temporary)
            throw ProfileError.unwritable(error.localizedDescription)
        }
    }

    func saveValidated(_ profiles: [SlicingProfile]) throws {
        try save(profiles)
    }

    private func parse(_ text: String) throws -> [SlicingProfile] {
        let lines = text.split(whereSeparator: \.isNewline).map(String.init)
        guard let versionLine = lines.first(where: { $0.trimmingCharacters(in: .whitespaces).hasPrefix("version:") }), scalar(versionLine) == "1" else { throw ProfileError.invalid("Unsupported profile version") }
        var profiles: [SlicingProfile] = []
        var current = SlicingProfile.defaultProfile
        var hasCurrent = false
        var section = ""
        for raw in lines {
            let line = raw.trimmingCharacters(in: .whitespaces)
            if line.isEmpty || line.hasPrefix("#") || line == "profiles:" { continue }
            if line.hasPrefix("- ") {
                if hasCurrent { profiles.append(try current.validated()) }
                current = SlicingProfile.defaultProfile
                current.name = value(line, key: "name") ?? current.name
                hasCurrent = true; section = ""; continue
            }
            guard let (key, value) = pair(line) else { continue }
            if raw.hasPrefix("    ") && !raw.hasPrefix("      ") { section = key; continue }
            switch section {
             case "slicing": if key == "layerHeight" { current.layerHeight = number(value) ?? current.layerHeight }
            case "output":
                 if key == "format" { current.outputFormat = value } else if key == "dpi" { current.outputDPI = Int(number(value) ?? Double(current.outputDPI)) } else if key == "combined" { current.combinedCricut = value == "true" } else if key == "layerNumbers" { current.showLayerNumbers = value == "true" } else if key == "layerNumberFontSize" { current.layerNumberFontSize = number(value) ?? current.layerNumberFontSize } else { current.outputOptions[key] = value }
            case "cleanup": if key == "mode" { current.cleanupMode = value } else if key == "featureWidth" { current.featureWidth = number(value) ?? current.featureWidth } else if key == "islandArea" { current.islandArea = number(value) ?? current.islandArea } else if key == "holeWidth" { current.holeWidth = number(value) ?? current.holeWidth } else if key == "bridgeWidth" { current.bridgeWidth = number(value) ?? current.bridgeWidth }
            case "cricut": if key == "packing" { current.packing = value } else if key == "gap" { current.cricutGap = number(value) ?? current.cricutGap } else if key == "guideInset" { current.guideInset = number(value) ?? current.guideInset } else if key == "layerNumberFontSize" { current.layerNumberFontSize = number(value) ?? current.layerNumberFontSize }
            default: break
            }
        }
        if hasCurrent { profiles.append(try current.validated()) }
        guard !profiles.isEmpty else { throw ProfileError.invalid("No profiles found") }
        return profiles
    }

    private func yaml(_ profiles: [SlicingProfile]) -> String {
        var result = "version: 1\nprofiles:\n"
        for p in profiles {
            result += "  - name: \(quote(p.name))\n    slicing:\n      layerHeight: \(p.layerHeight)\n    output:\n      format: \(quote(p.outputFormat))\n      dpi: \(p.outputDPI)\n      combined: \(p.combinedCricut)\n      layerNumbers: \(p.showLayerNumbers)\n"
            for key in p.outputOptions.keys.sorted() { result += "      \(key): \(quote(p.outputOptions[key] ?? ""))\n" }
            result += "    cleanup:\n      mode: \(quote(p.cleanupMode))\n      featureWidth: \(p.featureWidth)\n      islandArea: \(p.islandArea)\n      holeWidth: \(p.holeWidth)\n      bridgeWidth: \(p.bridgeWidth)\n    cricut:\n      packing: \(quote(p.packing))\n      gap: \(p.cricutGap)\n      guideInset: \(p.guideInset)\n"
             result += "      layerNumberFontSize: " + String(p.layerNumberFontSize) + "\n"
         }
         return result
    }

    private func pair(_ line: String) -> (String, String)? { guard let index = line.firstIndex(of: ":") else { return nil }; return (String(line[..<index]).trimmingCharacters(in: .whitespaces), unquote(String(line[line.index(after: index)...]).trimmingCharacters(in: .whitespaces))) }
    private func scalar(_ line: String) -> String { pair(line)?.1 ?? "" }
    private func value(_ line: String, key: String) -> String? { pair(String(line.dropFirst(2)))?.1 }
    private func number(_ value: String) -> Double? { Double(value) }
    private func quote(_ value: String) -> String { "\"" + value.replacingOccurrences(of: "\\", with: "\\\\").replacingOccurrences(of: "\"", with: "\\\"") + "\"" }
    private func unquote(_ value: String) -> String { value.count >= 2 && value.first == "\"" && value.last == "\"" ? String(value.dropFirst().dropLast()).replacingOccurrences(of: "\\\"", with: "\"").replacingOccurrences(of: "\\\\", with: "\\") : value }
}

final class ProfileController: ObservableObject {
    @Published private(set) var profiles: [SlicingProfile]
    @Published var selectedID: UUID
    @Published var errorMessage: String?
    private let store: ProfileStore

    init(store: ProfileStore = ProfileStore(), preferredID: UUID? = nil) {
        self.store = store
        let loaded = store.load()
        profiles = loaded
        selectedID = preferredID.flatMap { id in loaded.contains(where: { $0.id == id }) ? id : nil } ?? loaded[0].id
    }

    var activeProfile: SlicingProfile {
        get { profiles.first(where: { $0.id == selectedID }) ?? profiles[0] }
        set { update(newValue) }
    }

    func binding() -> Binding<SlicingProfile> {
        Binding(get: { self.activeProfile }, set: { self.update($0) })
    }

    func create() {
        let profile = SlicingProfile(name: uniqueName("New Profile"))
        profiles.append(profile); selectedID = profile.id; persist()
    }

    func duplicate() {
        var copy = activeProfile
        copy = SlicingProfile(id: UUID(), name: uniqueName("\(copy.name) Copy"), layerHeight: copy.layerHeight, outputFormat: copy.outputFormat, outputDPI: copy.outputDPI, combinedCricut: copy.combinedCricut, showLayerNumbers: copy.showLayerNumbers, layerNumberFontSize: copy.layerNumberFontSize, cleanupMode: copy.cleanupMode, featureWidth: copy.featureWidth, islandArea: copy.islandArea, holeWidth: copy.holeWidth, bridgeWidth: copy.bridgeWidth, packing: copy.packing, cricutGap: copy.cricutGap, guideInset: copy.guideInset, outputOptions: copy.outputOptions)
        profiles.append(copy); selectedID = copy.id; persist()
    }

    func deleteSelected() {
        guard profiles.count > 1 else { errorMessage = "The last profile cannot be deleted."; return }
        profiles.removeAll { $0.id == selectedID }; selectedID = profiles[0].id; persist()
    }

    private func update(_ profile: SlicingProfile) {
        guard let index = profiles.firstIndex(where: { $0.id == profile.id }) else { return }
        profiles[index] = profile; persist()
    }

    private func uniqueName(_ base: String) -> String {
        var candidate = base; var suffix = 2
        while profiles.contains(where: { $0.name == candidate }) { candidate = "\(base) \(suffix)"; suffix += 1 }
        return candidate
    }

    private func persist() {
        do { try store.save(profiles); errorMessage = nil }
        catch { errorMessage = error.localizedDescription }
    }
}
