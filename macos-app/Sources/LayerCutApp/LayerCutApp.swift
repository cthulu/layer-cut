import SwiftUI
import AppKit
import UniformTypeIdentifiers
import RealityKit

@main
struct LayerCutApp: App {
    @StateObject private var appSettings = AppSettingsController()
    @StateObject private var profiles = ProfileController(preferredID: AppSettingsController.load().defaultProfileID)

    var body: some SwiftUI.Scene {
        WindowGroup("Layer Cut") {
            ContentView()
                .frame(minWidth: 980, minHeight: 640)
                .background(WindowMaximizer())
                .environmentObject(appSettings).environmentObject(profiles)
                .preferredColorScheme(appSettings.settings.appearance.colorScheme)
        }
            .commands {
                CommandGroup(replacing: .newItem) {
                    Button("Open STL…") {
                        NotificationCenter.default.post(name: .layerCutOpenSTL, object: nil)
                    }
                    .keyboardShortcut("o", modifiers: .command)
                }
            }
        #if os(macOS)
        Settings { SettingsView().environmentObject(appSettings).environmentObject(profiles) }
        #endif
    }
}

private extension Notification.Name {
    static let layerCutOpenSTL = Notification.Name("LayerCut.openSTL")
}

private struct ContentView: View {
    private struct PreviewIdentity: Equatable {
        let path: String
        let profile: SlicingProfile
    }

    @EnvironmentObject private var appSettings: AppSettingsController
    @EnvironmentObject private var profiles: ProfileController
    @State private var modelName = "No STL loaded"
    @State private var modelPath: String?
    @State private var outputDirectory = "Default output directory"
    @State private var outputURL: URL?
    @State private var status = "Ready for an STL model"
    @State private var normalizedHeight = 0.0
    @State private var exportProgress = 0.0
    @State private var exportReport: ExportReport?
    @State private var exportError: String?
    @State private var pendingOverwritePaths: [String]?
    @State private var exportTask: Task<Void, Never>?
    @State private var layerOutput: SliceOutput?
    @State private var previewTask: Task<Void, Never>?
    @State private var layersExpanded = false
    @State private var layerOutputIdentity: PreviewIdentity?
    @State private var previewGeneration = UUID()
    @State private var previewZoom = 1.0
    @State private var previewError: String?
    @State private var previewProgress = 0.0
    @State private var isLayerPreviewPresented = false
    @State private var snapshot: MeshSnapshot?
    @State private var stackedSnapshot: MeshSnapshot?
    @State private var isGeneratingStackedPreview = false
    @State private var stackedPreviewProgress = 0.0
    @State private var stackedPreviewError: String?
    @State private var stackedPreviewTask: Task<Void, Never>?
    @State private var stackedPreviewGeneration = UUID()
    @State private var stackedPreviewIdentity: PreviewIdentity?
    @State private var activeLayer = 0
    @State private var resetCameraID = 0
    @State private var viewportError: String?
    @State private var cameraControlsExpanded = false
    @State private var stackedCameraControlsExpanded = false
    @State private var stackedResetCameraID = 0

    private let meshService = MeshService()

    var body: some View {
        NavigationSplitView {
            ParametersPanel(profileController: profiles, normalizedHeight: normalizedHeight, outputDirectory: outputDirectory, onOpenSTL: openStlPanel, onExport: beginExport, livePreview: $appSettings.settings.livePreview, onPreview: startStackedPreview, onPreviewLayers: presentLayerPreview, isGeneratingPreview: isGeneratingStackedPreview, previewProgress: stackedPreviewProgress, isExporting: exportTask != nil, progress: exportProgress, report: exportReport, status: status, onShowDiagnostics: { DiagnosticsWindowController.show(diagnostics) })
                .safeAreaInset(edge: .bottom) {
                    if let error = profiles.errorMessage { Text(error).font(.caption).foregroundStyle(.red).padding(8) }
                }
                .navigationTitle("Layer Cut")
                .navigationSplitViewColumnWidth(min: 360, ideal: 390, max: 440)
        } detail: {
             VStack(spacing: 0) {
                 HSplitView {
                     VStack(spacing: 0) {
                         Text("Original STL model").font(.headline).frame(maxWidth: .infinity, alignment: .leading).padding(8)
                         ZStack {
                             ViewportView(snapshot: snapshot, activeLayer: activeLayer,
                                          layerHeight: profiles.activeProfile.layerHeight,
                                          activeLayerZ: layerOutput?.layers.first(where: { $0.index == activeLayer })?.z,
                                          resetCameraID: resetCameraID, onLayerChange: { activeLayer = $0 })
                                 .frame(maxWidth: .infinity, maxHeight: .infinity)
                             if snapshot == nil {
                                 ViewportStateView(title: modelPath == nil ? "No model loaded" : "Loading original model…",
                                                   message: viewportError ?? (modelPath == nil ? "Open an STL to inspect the model." : "Preparing the engine mesh snapshot."),
                                                   systemImage: modelPath == nil ? "cube.transparent" : "hourglass")
                             }
                             CameraControls(expanded: $cameraControlsExpanded, activeLayer: $activeLayer,
                                            layerCount: layerCount, onResetCamera: { resetCameraID += 1 },
                                            onResetTransform: resetTransform)
                         }
                         .frame(minWidth: 360, minHeight: 240)
                     }
                     VStack(spacing: 0) {
                         Text("Stacked layer model").font(.headline).frame(maxWidth: .infinity, alignment: .leading).padding(8)
                         ZStack {
                             ViewportView(snapshot: stackedSnapshot, activeLayer: activeLayer,
                                          layerHeight: profiles.activeProfile.layerHeight, activeLayerZ: nil,
                                          resetCameraID: stackedResetCameraID, onLayerChange: { _ in })
                                 .frame(maxWidth: .infinity, maxHeight: .infinity)
                             if stackedSnapshot != nil {
                                 EmptyView()
                             } else if isGeneratingStackedPreview {
                                 ViewportStateView(title: "Stacked preview loading…", message: "Generating the engine stacked-layer snapshot.", systemImage: "hourglass")
                             } else {
                                 ViewportStateView(title: "Stacked preview unavailable", message: stackedPreviewError ?? "Enable live preview or generate a stacked preview from the sidebar.", systemImage: "cube.transparent")
                             }
                             if stackedSnapshot != nil {
                                 CameraControls(expanded: $stackedCameraControlsExpanded, activeLayer: $activeLayer,
                                                layerCount: layerCount, onResetCamera: { stackedResetCameraID += 1 },
                                                onResetTransform: resetTransform)
                             }
                         }
                         .frame(maxWidth: .infinity, maxHeight: .infinity)
                         .frame(minWidth: 360, minHeight: 240)
                     }
                 }
                Text("\(modelName)  |  \(status)").font(.callout).foregroundStyle(.secondary).padding(12)
             }.frame(maxWidth: .infinity, maxHeight: .infinity).background(.windowBackground)
         }
          .task(id: snapshotTaskKey) { await refreshSnapshots() }
          .onChange(of: profiles.activeProfile) { _, _ in
             layerOutput = nil
             layerOutputIdentity = nil
             previewError = nil
                cancelPreview()
              cancelStackedPreview()
               if appSettings.settings.livePreview { startStackedPreview() }
          }
          .onChange(of: appSettings.settings.livePreview) { _, enabled in
              if enabled { startStackedPreview() } else { cancelStackedPreview() }
          }
         .onReceive(NotificationCenter.default.publisher(for: .layerCutOpenSTL)) { _ in
             openStlPanel()
         }
         .sheet(isPresented: $isLayerPreviewPresented, onDismiss: cancelPreview) {
             LayerPreviewSheet(output: layerOutput, task: previewTask != nil, progress: previewProgress,
                               error: previewError, selection: Binding(get: { activeLayer }, set: { activeLayer = $0 ?? 0 }),
                               zoom: $previewZoom, onCancel: { isLayerPreviewPresented = false })
                 .frame(minWidth: 620, minHeight: 460)
         }
        .alert("Overwrite existing files?", isPresented: Binding(get: { pendingOverwritePaths != nil }, set: { if !$0 { pendingOverwritePaths = nil } })) {
            Button { pendingOverwritePaths = nil } label: { Label("Cancel", systemImage: "xmark") }
            Button(role: .destructive) {
                pendingOverwritePaths = nil
                startExport(allowOverwrite: true)
            } label: { Label("Overwrite", systemImage: "arrow.uturn.right") }
        } message: {
            Text("The export contains existing files: \((pendingOverwritePaths ?? []).joined(separator: ", "))")
        }
         .alert("Export Error", isPresented: Binding(get: { exportError != nil }, set: { if !$0 { exportError = nil } })) {
            Button { exportError = nil } label: { Label("OK", systemImage: "checkmark") }.keyboardShortcut(.defaultAction)
         } message: {
             Text(exportError ?? "Unknown export error")
         }
     }

    private var layerCount: Int {
        guard let snapshot else { return 1 }
        return max(1, Int(ceil(Double(snapshot.bounds.max.z - snapshot.bounds.min.z) / profiles.activeProfile.layerHeight)))
    }

    private var diagnostics: [ExportDiagnostic] {
        guard let report = exportReport else { return [] }
        return report.summary.diagnostics +
            report.failures.map { ExportDiagnostic(severity: .error, message: "\($0.path): \($0.message)") }
    }

    private func resetTransform() {
        var profile = profiles.activeProfile
        profile.axis = "+Z"
        profile.rotation = 0
        profile.scale = 1
        profiles.activeProfile = profile
    }

    private func openStlPanel() {
        let panel = NSOpenPanel(); panel.allowsMultipleSelection = false; panel.canChooseDirectories = false; panel.canChooseFiles = true
        panel.allowedContentTypes = [UTType(filenameExtension: "stl") ?? .data]
         if panel.runModal() == .OK, let url = panel.url {
             modelName = url.lastPathComponent
             appSettings.rememberSTL(url)
            modelPath = url.path
            snapshot = nil
            stackedSnapshot = nil
            stackedPreviewError = nil
            cancelStackedPreview()
            if appSettings.settings.livePreview { startStackedPreview() }
            activeLayer = 0
            layerOutput = nil
            layerOutputIdentity = nil
            cancelPreview()
            status = "Loading model snapshot…"
        }
    }

    private func beginExport() {
         let panel = NSOpenPanel(); panel.allowsMultipleSelection = false; panel.canChooseDirectories = true; panel.canChooseFiles = false
         panel.canCreateDirectories = true
         if let path = appSettings.settings.defaultOutputDirectory { panel.directoryURL = URL(fileURLWithPath: path, isDirectory: true) }
        guard panel.runModal() == .OK, let url = panel.url else { return }
        outputDirectory = url.path
        outputURL = url
        status = "Output directory selected"
        startExport(allowOverwrite: false)
    }

    private func presentLayerPreview() {
        isLayerPreviewPresented = true
        guard modelPath != nil else { previewError = "Open an STL model first."; return }
        startPreview()
    }

    private func startPreview() {
        guard let modelPath else { return }
        let profile = profiles.activeProfile
        let identity = PreviewIdentity(path: modelPath, profile: profile)
        guard layerOutputIdentity != identity else { return }
        previewTask?.cancel()
        let generation = UUID()
        previewGeneration = generation
        previewError = nil
        previewProgress = 0
        previewTask = Task {
            do {
                let value = try await SlicingService().preview(path: modelPath, profile: profile, dpi: 96,
                    progress: { value in Task { @MainActor in
                        guard generation == previewGeneration else { return }
                        previewProgress = value
                    } })
                await MainActor.run {
                    guard generation == previewGeneration,
                          self.modelPath == modelPath, profiles.activeProfile == profile else { return }
                    layerOutput = value
                    layerOutputIdentity = identity
                    activeLayer = min(activeLayer, max(0, value.layers.count - 1))
                    previewTask = nil
                    status = "Preview ready"
                }
            } catch is CancellationError {
                await MainActor.run {
                    if generation == previewGeneration { previewTask = nil }
                }
            } catch {
                await MainActor.run {
                    guard generation == previewGeneration else { return }
                    previewError = error.localizedDescription
                    previewTask = nil
                    status = "Preview failed"
                }
            }
        }
    }

    private func cancelPreview() {
        previewGeneration = UUID()
        previewTask?.cancel()
        previewTask = nil
    }

    private func startStackedPreview() {
        guard let modelPath else { return }
        let profile = profiles.activeProfile
        let identity = PreviewIdentity(path: modelPath, profile: profile)
        guard stackedPreviewIdentity != identity || stackedPreviewTask == nil else { return }
        stackedPreviewTask?.cancel()
        let generation = UUID()
        stackedPreviewGeneration = generation
        stackedPreviewIdentity = identity
        isGeneratingStackedPreview = true
        stackedPreviewProgress = 0
        stackedPreviewError = nil
        stackedPreviewTask = Task {
            do {
                let value = try await meshService.stackedSnapshot(path: modelPath, profile: profile,
                    progress: { value in Task { @MainActor in
                        guard generation == stackedPreviewGeneration else { return }
                        stackedPreviewProgress = value
                    } })
                await MainActor.run {
                    guard generation == stackedPreviewGeneration, self.modelPath == modelPath, profiles.activeProfile == profile else { return }
                    stackedSnapshot = value
                    isGeneratingStackedPreview = false
                    stackedPreviewTask = nil
                    status = "Ready"
                }
            } catch is CancellationError {
                await MainActor.run { if generation == stackedPreviewGeneration { isGeneratingStackedPreview = false; stackedPreviewTask = nil } }
            } catch {
                await MainActor.run {
                    guard generation == stackedPreviewGeneration else { return }
                    stackedSnapshot = nil
                    stackedPreviewError = error.localizedDescription
                    isGeneratingStackedPreview = false
                    stackedPreviewTask = nil
                    status = "Stacked preview failed"
                }
            }
        }
    }

    private func cancelStackedPreview() {
        stackedPreviewGeneration = UUID()
        stackedPreviewTask?.cancel()
        stackedPreviewTask = nil
        isGeneratingStackedPreview = false
        stackedPreviewProgress = 0
        stackedPreviewIdentity = nil
    }

    private var snapshotTaskKey: String {
        guard let modelPath else { return "none" }
        let profile = profiles.activeProfile
        return "\(modelPath)|\(profile.axis)|\(profile.rotation)|\(profile.scale)"
    }

    @MainActor
    private func refreshSnapshots() async {
        guard let modelPath else { return }
        let profile = profiles.activeProfile
        do {
            async let metadata = meshService.load(path: modelPath)
            async let transformed = meshService.snapshot(path: modelPath, axis: profile.axis, rotation: profile.rotation, scale: profile.scale)
             let (meshMetadata, value) = try await (metadata, transformed)
             normalizedHeight = Double(meshMetadata.bounds.max.z - meshMetadata.bounds.min.z)
             snapshot = value
            activeLayer = 0
            status = "Ready"
            viewportError = nil
        } catch is CancellationError {
            // A profile transform changed while the previous snapshot was loading.
        } catch {
            viewportError = error.localizedDescription
            stackedSnapshot = nil
            stackedPreviewError = error.localizedDescription
            status = "Unable to load model"
        }
    }

    private func startExport(allowOverwrite: Bool) {
        guard let modelPath, let outputURL else { exportError = "Choose an STL and output directory first."; return }
        let profile = profiles.activeProfile
        exportProgress = 0
        exportReport = nil
        status = "Exporting…"
        exportTask = Task {
            do {
                let report = try await ExportCoordinator().run(path: modelPath, profile: profile,
                    directory: SecurityScopedDirectory(url: outputURL), allowOverwrite: allowOverwrite,
                    progress: { value in Task { @MainActor in exportProgress = value } })
                await MainActor.run {
                    exportReport = report
                    status = report.completion == .completed ? "Export complete" : "Export completed with failures"
                    exportTask = nil
                }
            } catch let error as ExportError {
                await MainActor.run {
                    if case .overwriteConfirmationRequired(let paths) = error { pendingOverwritePaths = paths }
                    else { exportError = error.localizedDescription }
                    status = {
                        if case .cancelled = error { return "Export cancelled" }
                        return "Export not completed"
                    }()
                    exportTask = nil
                }
            } catch {
                await MainActor.run { exportError = error.localizedDescription; status = "Export not completed"; exportTask = nil }
            }
        }
    }
}

private struct SettingsView: View {
    @EnvironmentObject private var appSettings: AppSettingsController
    @EnvironmentObject private var profiles: ProfileController

    var body: some View {
        TabView {
            Form {
                Section("Profiles") {
                    Picker("Default profile", selection: Binding(get: { appSettings.settings.defaultProfileID ?? profiles.activeProfile.id }, set: { id in appSettings.settings.defaultProfileID = id; profiles.selectedID = id })) {
                        ForEach(profiles.profiles) { Text($0.name).tag($0.id) }
                    }
                    Text("Profiles are stored as versioned YAML in the application-support directory.").font(.caption).foregroundStyle(.secondary)
                }
                Section("Export") {
                    HStack { Text(appSettings.settings.defaultOutputDirectory ?? "No default directory").lineLimit(1); Spacer(); Button("Choose…", action: chooseOutputDirectory) }
                    if appSettings.settings.defaultOutputDirectory != nil { Button("Clear default directory", role: .destructive) { appSettings.settings.defaultOutputDirectory = nil } }
                }
                Section("Recent STL documents") {
                    if appSettings.settings.recentSTLPaths.isEmpty { Text("No recent STL documents.").foregroundStyle(.secondary) }
                    ForEach(appSettings.settings.recentSTLPaths, id: \.self) { path in HStack { Text(URL(fileURLWithPath: path).lastPathComponent); Spacer(); Button("Remove") { appSettings.removeRecentSTL(path) } } }
                }
                Section("Appearance") { Picker("Appearance", selection: $appSettings.settings.appearance) { ForEach(AppearancePreference.allCases, id: \.self) { Text($0.label).tag($0) } }.pickerStyle(.segmented) }
            }.formStyle(.grouped).tabItem { Label("General", systemImage: "gear") }
            AboutView().tabItem { Label("About", systemImage: "info.circle") }
        }.padding().frame(width: 560, height: 500)
    }

    private func chooseOutputDirectory() {
        let panel = NSOpenPanel(); panel.canChooseDirectories = true; panel.canChooseFiles = false; panel.canCreateDirectories = true
        if panel.runModal() == .OK, let url = panel.url { appSettings.settings.defaultOutputDirectory = url.path }
    }
}

private struct WindowMaximizer: NSViewRepresentable {
    final class Coordinator {
        var didMaximize = false
    }

    func makeCoordinator() -> Coordinator { Coordinator() }
    func makeNSView(context: Context) -> NSView { NSView() }

    func updateNSView(_ nsView: NSView, context: Context) {
        DispatchQueue.main.async {
            guard !context.coordinator.didMaximize,
                  let window = nsView.window, !window.isZoomed else { return }
            context.coordinator.didMaximize = true
            window.zoom(nil)
        }
    }
}

private struct ViewportStateView: View {
    let title: String
    let message: String
    let systemImage: String

    var body: some View {
        ContentUnavailableView(title, systemImage: systemImage, description: Text(message))
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(.windowBackground.opacity(0.92))
            .accessibilityElement(children: .combine)
            .accessibilityLabel("Viewport: \(title). \(message)")
    }
}

private struct CameraControls: View {
    @Binding var expanded: Bool
    @Binding var activeLayer: Int
    let layerCount: Int
    let onResetCamera: () -> Void
    let onResetTransform: () -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Button { withAnimation(.easeInOut(duration: 0.15)) { expanded.toggle() } } label: {
                Label(expanded ? "Hide camera controls" : "Show camera controls",
                      systemImage: expanded ? "chevron.up.circle" : "camera.rotate")
            }
            .buttonStyle(.borderless)
            .accessibilityHint(expanded ? "Hides camera and layer controls" : "Shows camera help, reset, and layer controls")
            if expanded {
                Text("Cura-like controls: right-drag orbits around the model; middle-drag pans; scroll zooms. Horizontal drag changes yaw, vertical drag changes pitch.")
                    .fixedSize(horizontal: false, vertical: true)
                HStack {
                    Button("Reset camera", action: onResetCamera)
                    Button("Reset transform", action: onResetTransform)
                }
                Stepper("Layer \(min(activeLayer + 1, layerCount)) / \(layerCount)", value: $activeLayer, in: 0...max(0, layerCount - 1))
            }
        }
        .font(.caption)
        .padding(8)
        .background(.thinMaterial, in: RoundedRectangle(cornerRadius: 8))
        .padding()
        .frame(maxWidth: 390, alignment: .leading)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    }
}

private struct DiagnosticsSheet: View {
    let diagnostics: [ExportDiagnostic]

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Export diagnostics").font(.title2.weight(.semibold))
                Spacer()
                Text("\(diagnostics.count) message(s)").foregroundStyle(.secondary)
            }
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 10) {
                    ForEach(diagnostics) { diagnostic in
                        VStack(alignment: .leading, spacing: 3) {
                            Label(diagnostic.severity == .error ? "Error" : "Warning",
                                  systemImage: diagnostic.severity == .error ? "xmark.octagon.fill" : "exclamationmark.triangle.fill")
                                .foregroundStyle(diagnostic.severity == .error ? .red : .orange)
                                .accessibilityLabel(diagnostic.severity == .error ? "Export error" : "Export warning")
                            Text(diagnostic.message).textSelection(.enabled)
                        }
                        .padding(8)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .background(.quaternary, in: RoundedRectangle(cornerRadius: 6))
                    }
                }
            }
        }
        .padding(20)
        .accessibilityElement(children: .contain)
    }
}

private enum DiagnosticsWindowController {
    private static var window: NSWindow?

    static func show(_ diagnostics: [ExportDiagnostic]) {
        if let window {
            window.contentViewController = NSHostingController(rootView: DiagnosticsSheet(diagnostics: diagnostics))
            window.makeKeyAndOrderFront(nil)
            return
        }
        let window = NSWindow(contentViewController: NSHostingController(rootView: DiagnosticsSheet(diagnostics: diagnostics)))
        window.title = "Export diagnostics"
        window.styleMask = [.titled, .closable, .resizable]
        window.setContentSize(NSSize(width: 560, height: 400))
        window.center()
        window.isReleasedWhenClosed = false
        window.makeKeyAndOrderFront(nil)
        self.window = window
    }
}

private struct AboutView: View {
    private let licenses = "Clipper2: Zlib License\nstb_image_write: Public Domain\nCLI11: BSD 3-Clause License\nApple SwiftUI, RealityKit: Apple system frameworks"
    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            Text("Layer Cut").font(.title).bold()
            Text("Version \(Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "0.1.0")")
            Text("Engine version 0 (C++17)").foregroundStyle(.secondary)
            Divider(); Text("Third-party notices").font(.headline); Text(licenses).font(.caption.monospaced()); Spacer()
        }.frame(maxWidth: .infinity, alignment: .leading).padding()
    }
}
