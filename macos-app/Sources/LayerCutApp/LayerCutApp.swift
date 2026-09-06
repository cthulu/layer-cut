import SwiftUI
import AppKit
import UniformTypeIdentifiers
import RealityKit

@main
struct LayerCutApp: App {
    var body: some SwiftUI.Scene {
        WindowGroup("Layer Cut") {
            ContentView()
                .frame(minWidth: 980, minHeight: 640)
        }

        #if os(macOS)
        Settings {
            SettingsView()
        }
        #endif
    }
}

private struct ContentView: View {
    @State private var modelName = "No STL loaded"
    @State private var outputDirectory = "Default output directory"
    @State private var profileName = "Default"
    @State private var layerHeight = 1.0
    @State private var scale = 1.0
    @State private var selectedAxis = "+Z"
    @State private var rotation = 0.0
    @State private var status = "Ready for an STL model"

    private let axes = ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]

    var body: some View {
        NavigationSplitView {
            Form {
                Section("Profile") {
                    HStack {
                        Picker("Active profile", selection: $profileName) {
                            Text("Default").tag("Default")
                        }
                        Button {
                            status = "Profile creation will be connected to YAML storage"
                        } label: {
                            Image(systemName: "plus")
                        }
                        .buttonStyle(.borderless)
                        .help("Create profile")
                    }
                }

                Section("Model") {
                    Button("Open STL…") { openStlPanel() }
                    Text(modelName)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .lineLimit(2)
                }

                Section("Orientation") {
                    Picker("Up axis", selection: $selectedAxis) {
                        ForEach(axes, id: \.self) { Text($0) }
                    }
                    Slider(value: Binding(
                        get: { rotation },
                        set: { rotation = $0.rounded() }
                    ), in: -180...180) {
                        Text("Rotation")
                    } minimumValueLabel: {
                        Text("-180°")
                    } maximumValueLabel: {
                        Text("180°")
                    }
                    LabeledContent("Rotation", value: "\(Int(rotation))°")
                    Slider(value: Binding(
                        get: { scale },
                        set: { scale = (round($0 * 100) / 100).clamped(to: 0.1...2.0) }
                    ), in: 0.1...2.0) {
                        Text("Scale")
                    }
                    LabeledContent("Scale", value: String(format: "%.2fx", scale))
                }

                Section("Slicing") {
                    Slider(value: Binding(
                        get: { layerHeight },
                        set: { layerHeight = (round($0 * 10) / 10).clamped(to: 0.1...5.0) }
                    ), in: 0.1...5.0) {
                        Text("Layer height")
                    }
                    LabeledContent("Layer height", value: String(format: "%.1f mm", layerHeight))
                }

                Section("Output") {
                    Button("Choose Output Folder…") { openOutputFolderPanel() }
                    Text(outputDirectory)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .lineLimit(2)
                    Button("Preview Configuration") {
                        status = "Preview will use the active profile and transformed model"
                    }
                    Button("Export") {
                        status = "Engine export integration is the next implementation step"
                    }
                    .buttonStyle(.borderedProminent)
                }
            }
            .formStyle(.grouped)
            .padding()
            .frame(minWidth: 360, idealWidth: 390, maxWidth: 440)
            .navigationTitle("Layer Cut")
            .navigationSplitViewColumnWidth(min: 360, ideal: 390, max: 440)
        } detail: {
            VStack(spacing: 0) {
                ViewportView()
                    .overlay(alignment: .topLeading) {
                        Text("Right-drag to orbit")
                            .font(.caption)
                            .padding(8)
                            .background(.thinMaterial, in: Capsule())
                            .padding()
                    }
                Divider()
                Text(status)
                    .font(.callout)
                    .foregroundStyle(.secondary)
                    .padding(12)
            }
             .frame(maxWidth: .infinity, maxHeight: .infinity)
             .background(.windowBackground)
         }
     }

     private func openStlPanel() {
         let panel = NSOpenPanel()
         panel.allowsMultipleSelection = false
         panel.canChooseDirectories = false
         panel.canChooseFiles = true
         panel.allowedContentTypes = [UTType(filenameExtension: "stl") ?? .data]

         switch panel.runModal() {
         case .OK:
             let url = panel.url ?? URL(fileURLWithPath: NSHomeDirectory())
             modelName = url.lastPathComponent
             status = "Loaded \(modelName); engine metadata is not connected yet"
         default:
             break
          }
      }

     private func openOutputFolderPanel() {
         let panel = NSOpenPanel()
         panel.allowsMultipleSelection = false
         panel.canChooseDirectories = true
         panel.canChooseFiles = false

         switch panel.runModal() {
         case .OK:
             if let url = panel.url {
                outputDirectory = url.path
                status = "Output directory selected"
             }
         default:
             break
          }
      }
 }

private extension BinaryFloatingPoint {
    func clamped(to range: ClosedRange<Self>) -> Self {
        Swift.min(Swift.max(self, range.lowerBound), range.upperBound)
    }
}

private struct SettingsView: View {
    var body: some View {
        Form {
            Text("YAML profile storage will be connected in the profile implementation step.")
                .foregroundStyle(.secondary)
        }
        .padding()
        .frame(width: 420)
    }
}
