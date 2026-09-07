import AppKit
import RealityKit
import SwiftUI

/// The viewport uses the engine's millimetres. X and Y are the horizontal axes
/// and Z is up; no STL data or transform is recreated here.
struct ViewportView: NSViewRepresentable {
    let snapshot: MeshSnapshot?
    let activeLayer: Int
    let layerHeight: Double
    let activeLayerZ: Double?
    let resetCameraID: Int
    let onLayerChange: (Int) -> Void

    func makeNSView(context: Context) -> OrbitARView {
        let view = OrbitARView(frame: .zero)
        view.onLayerChange = onLayerChange
        view.update(snapshot: snapshot, activeLayer: activeLayer, layerHeight: layerHeight, activeLayerZ: activeLayerZ, resetCameraID: resetCameraID)
        return view
    }

    func updateNSView(_ nsView: OrbitARView, context: Context) {
        nsView.onLayerChange = onLayerChange
        nsView.update(snapshot: snapshot, activeLayer: activeLayer, layerHeight: layerHeight, activeLayerZ: activeLayerZ, resetCameraID: resetCameraID)
    }
}

/// RealityKit-specific conversion and camera interaction live behind this
/// container. The engine remains the only owner of mesh topology and transforms.
final class OrbitARView: ARView {
    private let cameraAnchor = AnchorEntity(world: .zero)
    private let orbitCamera = PerspectiveCamera()
    private let contentAnchor = AnchorEntity(world: .zero)
    private var modelEntity: ModelEntity?
    private var planeEntity: ModelEntity?
    private var lastPoint: CGPoint?
    private var yaw = Float.zero
    private var pitch = Float.zero
    private var pan = SIMD3<Float>.zero
    private var distance: Float = 100
    private var lastSnapshot: MeshSnapshot?
    private var lastLayer = -1
    private var lastLayerHeight = 1.0
    private var lastResetCameraID = 0

    var onLayerChange: ((Int) -> Void)?

    required init(frame: CGRect) {
        super.init(frame: frame)
        cameraAnchor.addChild(orbitCamera)
        scene.addAnchor(cameraAnchor)
        scene.addAnchor(contentAnchor)
        resetCamera()
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) is not supported")
    }

    override var acceptsFirstResponder: Bool { true }

    func update(snapshot: MeshSnapshot?, activeLayer: Int, layerHeight: Double, activeLayerZ: Double?, resetCameraID: Int) {
        if resetCameraID != lastResetCameraID {
            lastResetCameraID = resetCameraID
            resetCamera()
        }
        lastLayerHeight = layerHeight
        if snapshot != lastSnapshot {
            lastSnapshot = snapshot
            replaceModel(with: snapshot)
            if let snapshot { distance = cameraDistance(for: snapshot.bounds) }
        }
        if activeLayer != lastLayer || snapshot != nil {
            lastLayer = activeLayer
            updateSlicePlane(snapshot: snapshot, activeLayer: activeLayer, activeLayerZ: activeLayerZ)
        }
        updateOrbitCamera()
    }

    func resetCamera() {
        yaw = 0
        pitch = 0
        pan = .zero
        distance = lastSnapshot.map { cameraDistance(for: $0.bounds) } ?? 100
        updateOrbitCamera()
    }

    private func replaceModel(with snapshot: MeshSnapshot?) {
        modelEntity?.removeFromParent()
        modelEntity = nil
        guard let snapshot, let mesh = RealityKitMeshAdapter.mesh(from: snapshot) else { return }
        let entity = ModelEntity(mesh: mesh, materials: [SimpleMaterial(color: .systemBlue, isMetallic: false)])
        contentAnchor.addChild(entity)
        modelEntity = entity
    }

    private func updateSlicePlane(snapshot: MeshSnapshot?, activeLayer: Int, activeLayerZ: Double?) {
        planeEntity?.removeFromParent()
        planeEntity = nil
        guard let snapshot else { return }
        let width = max(snapshot.bounds.max.x - snapshot.bounds.min.x, 1)
        let depth = max(snapshot.bounds.max.y - snapshot.bounds.min.y, 1)
        guard let mesh = RealityKitMeshAdapter.plane(width: width * 1.08, depth: depth * 1.08) else { return }
        let plane = ModelEntity(mesh: mesh, materials: [SimpleMaterial(color: .systemOrange.withAlphaComponent(0.28), isMetallic: false)])
        let z = activeLayerZ.map(Float.init) ?? snapshot.bounds.min.z + Float(activeLayer) * Float(lastLayerHeight)
        plane.position = SIMD3<Float>(0, 0, z)
        contentAnchor.addChild(plane)
        planeEntity = plane
    }

    private func updateOrbitCamera() {
        let yawRotation = simd_quatf(angle: yaw, axis: SIMD3<Float>(0, 0, 1))
        let pitchRotation = simd_quatf(angle: pitch, axis: SIMD3<Float>(1, 0, 0))
        let rotation = yawRotation * pitchRotation
        let position = pan + rotation.act(SIMD3<Float>(0, 0, distance))
        orbitCamera.look(at: pan, from: position, relativeTo: nil)
    }

    private func cameraDistance(for bounds: (min: SIMD3<Float>, max: SIMD3<Float>)) -> Float {
        let size = bounds.max - bounds.min
        return max(length(size) * 1.8, 10)
    }

    override func rightMouseDown(with event: NSEvent) {
        window?.makeFirstResponder(self)
        lastPoint = convert(event.locationInWindow, from: nil)
    }

    override func rightMouseDragged(with event: NSEvent) {
        orbit(with: convert(event.locationInWindow, from: nil))
    }

    override func rightMouseUp(with event: NSEvent) { lastPoint = nil }

    override func otherMouseDown(with event: NSEvent) {
        lastPoint = convert(event.locationInWindow, from: nil)
    }

    override func otherMouseDragged(with event: NSEvent) {
        let point = convert(event.locationInWindow, from: nil)
        guard let previous = lastPoint else { lastPoint = point; return }
        let delta = point - previous
        lastPoint = point
        let speed = max(distance * 0.002, 0.05)
        pan += SIMD3<Float>(Float(delta.x) * speed, -Float(delta.y) * speed, 0)
        updateOrbitCamera()
    }

    override func otherMouseUp(with event: NSEvent) { lastPoint = nil }

    override func scrollWheel(with event: NSEvent) {
        distance *= pow(0.92, Float(event.scrollingDeltaY))
        distance = min(max(distance, 1), 100_000)
        updateOrbitCamera()
    }

    private func orbit(with point: CGPoint) {
        guard let previous = lastPoint else { lastPoint = point; return }
        let delta = point - previous
        lastPoint = point
        yaw += Float(delta.x) * 0.01
        pitch = min(max(pitch + Float(delta.y) * 0.01, -1.5), 1.5)
        updateOrbitCamera()
    }
}

/// Converts the plain C ABI snapshot into RealityKit resources. This is the
/// only RealityKit mesh conversion boundary in the application.
enum RealityKitMeshAdapter {
    static func mesh(from snapshot: MeshSnapshot) -> MeshResource? {
        let points = stride(from: 0, to: snapshot.vertices.count, by: 3).map {
            SIMD3<Float>(snapshot.vertices[$0], snapshot.vertices[$0 + 1], snapshot.vertices[$0 + 2])
        }
        guard !points.isEmpty, snapshot.indices.count.isMultiple(of: 3) else { return nil }
        var normals = Array(repeating: SIMD3<Float>.zero, count: points.count)
        for index in stride(from: 0, to: snapshot.indices.count, by: 3) {
            let first = Int(snapshot.indices[index])
            let second = Int(snapshot.indices[index + 1])
            let third = Int(snapshot.indices[index + 2])
            guard first < points.count, second < points.count, third < points.count else { return nil }
            let normal = simd_normalize(simd_cross(points[second] - points[first], points[third] - points[first]))
            normals[first] = normal
            normals[second] = normal
            normals[third] = normal
        }
        var descriptor = MeshDescriptor(name: "engine-snapshot")
        descriptor.positions = MeshBuffers.Positions(points)
        descriptor.normals = MeshBuffers.Normals(normals)
        descriptor.primitives = .triangles(snapshot.indices)
        return try? MeshResource.generate(from: [descriptor])
    }

    static func plane(width: Float, depth: Float) -> MeshResource? {
        let halfWidth = width / 2
        let halfDepth = depth / 2
        var descriptor = MeshDescriptor(name: "active-slice-plane")
        descriptor.positions = MeshBuffers.Positions([
            SIMD3(-halfWidth, -halfDepth, 0), SIMD3(halfWidth, -halfDepth, 0),
            SIMD3(halfWidth, halfDepth, 0), SIMD3(-halfWidth, halfDepth, 0)
        ])
        descriptor.primitives = .triangles([0, 1, 2, 0, 2, 3])
        return try? MeshResource.generate(from: [descriptor])
    }
}

private extension CGPoint {
    static func - (lhs: CGPoint, rhs: CGPoint) -> CGPoint {
        CGPoint(x: lhs.x - rhs.x, y: lhs.y - rhs.y)
    }
}
