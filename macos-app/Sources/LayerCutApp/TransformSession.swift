import Foundation

struct TransformSession: Equatable, Sendable {
    static let axes = ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]
    static let angleRange = -180.0...180.0
    static let scaleRange = 0.1...10.0

    var cuttingAxis = "+Z"
    var rotateX = 0.0
    var rotateY = 0.0
    var rotateZ = 0.0
    var scale = 1.0

    static let identity = TransformSession()

    var cacheKey: String {
        "axis=\(cuttingAxis);rx=\(rotateX);ry=\(rotateY);rz=\(rotateZ);scale=\(scale)"
    }

    func validated() -> TransformSession {
        var value = self
        if !Self.axes.contains(value.cuttingAxis) { value.cuttingAxis = "+Z" }
        value.rotateX = min(max(value.rotateX.isFinite ? value.rotateX : 0, -180), 180)
        value.rotateY = min(max(value.rotateY.isFinite ? value.rotateY : 0, -180), 180)
        value.rotateZ = min(max(value.rotateZ.isFinite ? value.rotateZ : 0, -180), 180)
        value.scale = min(max(value.scale.isFinite ? value.scale : 1, Self.scaleRange.lowerBound), Self.scaleRange.upperBound)
        return value
    }
}
