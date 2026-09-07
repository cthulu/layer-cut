import Foundation

@main
struct AppSettingsTests {
    static func main() throws {
        let suiteName = "layer-cut-tests-\(UUID().uuidString)"
        let defaults = UserDefaults(suiteName: suiteName)!
        defer { defaults.removePersistentDomain(forName: suiteName) }
        let controller = AppSettingsController(defaults: defaults)
        let first = URL(fileURLWithPath: "/tmp/one.stl")
        let second = URL(fileURLWithPath: "/tmp/two.stl")
        controller.rememberSTL(first)
        controller.rememberSTL(second)
        controller.rememberSTL(first)
        precondition(controller.settings.recentSTLPaths == [first.path, second.path])
        controller.settings.appearance = .dark
        let reloaded = AppSettingsController(defaults: defaults)
        precondition(reloaded.settings.appearance == .dark)
        reloaded.removeRecentSTL(second.path)
        precondition(reloaded.settings.recentSTLPaths == [first.path])
    }
}
