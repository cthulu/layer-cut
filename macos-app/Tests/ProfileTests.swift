import Foundation

@main
struct ProfileTests {
    static func main() throws {
        try testDefaultProfileMatchesSpec()
        try testUnknownOutputOptionRoundTrips()
        try testInvalidFileRecoversToDefault()
        try testValidationRejectsDuplicateNames()
        try testTransformIsNotPersisted()
        try testTransformCacheKeyIncludesEveryValue()
    }

    private static func testDefaultProfileMatchesSpec() throws {
        let profile = SlicingProfile.defaultProfile
        precondition(profile.name == "Default" && profile.layerHeight == 1)
        precondition(profile.outputFormat == "cricut-normal" && profile.packing == "fixed")
        precondition(profile.cricutGap == 3 && profile.guideInset == 1 && profile.cleanupMode == "warn")
        precondition(!profile.showLayerNumbers)
        precondition(profile.layerNumberFontSize == 2.5)
        _ = try profile.validated()
        let transform = TransformSession.identity
        precondition(transform.cuttingAxis == "+Z" && transform.rotateX == 0 && transform.rotateY == 0 && transform.rotateZ == 0 && transform.scale == 1)
        var changed = TransformSession(cuttingAxis: "-Y", rotateX: 181, rotateY: -181, rotateZ: 12, scale: 0)
        changed = changed.validated()
        precondition(changed.cuttingAxis == "-Y" && changed.rotateX == 180 && changed.rotateY == -180 && changed.rotateZ == 12 && changed.scale == TransformSession.scaleRange.lowerBound)
    }

    private static func testUnknownOutputOptionRoundTrips() throws {
        let url = temporaryURL("roundtrip")
        defer { try? FileManager.default.removeItem(at: url) }
        var profile = SlicingProfile.defaultProfile
        profile.showLayerNumbers = true
        profile.layerNumberFontSize = 1.75
        profile.outputOptions = ["futureTolerance": "0.25", "newFlag": "true"]
        let store = ProfileStore(fileURL: url)
        try store.save([profile])
        precondition(store.load().first?.outputOptions == profile.outputOptions)
        precondition(store.load().first?.showLayerNumbers == true)
        precondition(store.load().first?.layerNumberFontSize == 1.75)
        let contents = try String(contentsOf: url)
        precondition(contents.contains("futureTolerance") && contents.contains("layerNumbers: true") && contents.contains("layerNumberFontSize: 1.75"))
    }

    private static func testInvalidFileRecoversToDefault() throws {
        let url = temporaryURL("invalid")
        defer { try? FileManager.default.removeItem(at: url) }
        try "version: 999\nprofiles: []\n".write(to: url, atomically: true, encoding: .utf8)
        let recovered = ProfileStore(fileURL: url).load()
        precondition(recovered.count == 1 && recovered[0].name == "Default")
    }

    private static func testValidationRejectsDuplicateNames() throws {
        let url = temporaryURL("duplicate")
        defer { try? FileManager.default.removeItem(at: url) }
        do {
            try ProfileStore(fileURL: url).save([.defaultProfile, SlicingProfile(name: "Default")])
            preconditionFailure("duplicate profile names must be rejected")
        } catch { }
    }

    private static func testTransformIsNotPersisted() throws {
        let url = temporaryURL("transform-session")
        defer { try? FileManager.default.removeItem(at: url) }
        var transform = TransformSession.identity
        transform.cuttingAxis = "-X"
        transform.rotateX = 45
        transform.scale = 2
        try ProfileStore(fileURL: url).save([.defaultProfile])
        let contents = try String(contentsOf: url)
        precondition(transform != .identity)
        precondition(!contents.contains("orientation") && !contents.contains("rotateX"))
        precondition(ProfileStore(fileURL: url).load().first == .defaultProfile)
    }

    private static func testTransformCacheKeyIncludesEveryValue() {
        let baseline = TransformSession.identity.cacheKey
        for change in [
            TransformSession(cuttingAxis: "-X"),
            TransformSession(rotateX: 1),
            TransformSession(rotateY: 1),
            TransformSession(rotateZ: 1),
            TransformSession(scale: 2)
        ] {
            precondition(change.cacheKey != baseline)
        }
    }

    private static func temporaryURL(_ name: String) -> URL {
        FileManager.default.temporaryDirectory.appendingPathComponent("layer-cut-profile-\(name)-\(UUID().uuidString).yaml")
    }
}
