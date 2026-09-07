import Foundation
import SwiftUI

enum AppearancePreference: String, CaseIterable, Codable {
    case system, light, dark
    var label: String { rawValue.capitalized }
    var colorScheme: ColorScheme? { self == .system ? nil : (self == .light ? .light : .dark) }
}

struct AppSettings: Codable, Equatable {
    var defaultProfileID: UUID?
    var defaultOutputDirectory: String?
    var recentSTLPaths: [String]
    var appearance: AppearancePreference
    static let `default` = AppSettings(defaultProfileID: nil, defaultOutputDirectory: nil, recentSTLPaths: [], appearance: .system)
}

final class AppSettingsController: ObservableObject {
    @Published var settings: AppSettings { didSet { save() } }
    private let defaults: UserDefaults
    private let key = "layer-cut.app-settings.v1"

    static func load() -> AppSettings {
        guard let data = UserDefaults.standard.data(forKey: "layer-cut.app-settings.v1"), let value = try? JSONDecoder().decode(AppSettings.self, from: data) else { return .default }
        return value
    }

    init(defaults: UserDefaults = .standard) {
        self.defaults = defaults
        if let data = defaults.data(forKey: key), let value = try? JSONDecoder().decode(AppSettings.self, from: data) { settings = value } else { settings = .default }
    }

    func rememberSTL(_ url: URL) {
        let path = url.standardizedFileURL.path
        settings.recentSTLPaths = Array(([path] + settings.recentSTLPaths.filter { $0 != path }).prefix(10))
    }

    func removeRecentSTL(_ path: String) { settings.recentSTLPaths.removeAll { $0 == path } }

    private func save() {
        guard let data = try? JSONEncoder().encode(settings) else { return }
        defaults.set(data, forKey: key)
    }
}
