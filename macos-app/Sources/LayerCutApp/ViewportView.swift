import AppKit
import RealityKit
import SwiftUI

/// RealityKit viewport with Cura/Fusion-style secondary-button orbiting.
struct ViewportView: NSViewRepresentable {
    func makeNSView(context: Context) -> OrbitARView {
        OrbitARView(frame: .zero)
    }

    func updateNSView(_ nsView: OrbitARView, context: Context) {}
}

final class OrbitARView: ARView {
    private let cameraAnchor = AnchorEntity(world: .zero)
    private let orbitCamera = PerspectiveCamera()
    private var lastOrbitPoint: CGPoint?
    private var yaw = Float.zero
    private var pitch = Float.zero
    private let distance: Float = 4

    required init(frame: CGRect) {
        super.init(frame: frame)
        cameraAnchor.addChild(orbitCamera)
        scene.addAnchor(cameraAnchor)
        updateOrbitCamera()
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) is not supported")
    }

    override var acceptsFirstResponder: Bool { true }

    override func rightMouseDown(with event: NSEvent) {
        window?.makeFirstResponder(self)
        lastOrbitPoint = convert(event.locationInWindow, from: nil)
    }

    override func rightMouseDragged(with event: NSEvent) {
        let point = convert(event.locationInWindow, from: nil)
        guard let lastOrbitPoint else {
            self.lastOrbitPoint = point
            return
        }

        let delta = point - lastOrbitPoint
        self.lastOrbitPoint = point
        yaw -= Float(delta.x) * 0.01
        pitch = min(max(pitch - Float(delta.y) * 0.01, -1.45), 1.45)
        updateOrbitCamera()
    }

    override func rightMouseUp(with event: NSEvent) {
        lastOrbitPoint = nil
    }

    private func updateOrbitCamera() {
        let yawRotation = simd_quatf(angle: yaw, axis: SIMD3<Float>(0, 1, 0))
        let pitchRotation = simd_quatf(angle: pitch, axis: SIMD3<Float>(1, 0, 0))
        let rotation = yawRotation * pitchRotation
        let position = rotation.act(SIMD3<Float>(0, 0, distance))
        orbitCamera.transform = Transform(rotation: rotation, translation: position)
    }
}

private extension CGPoint {
    static func - (lhs: CGPoint, rhs: CGPoint) -> CGPoint {
        CGPoint(x: lhs.x - rhs.x, y: lhs.y - rhs.y)
    }
}
