import SwiftUI

struct ParametersPanel: View {
    @ObservedObject var profileController: ProfileController
    @State private var profileExpanded = true
    @State private var orientationExpanded = true
    @State private var slicingExpanded = true
    @State private var cleanupExpanded = false
    @State private var outputExpanded = true
    @State private var cricutExpanded = true
    let normalizedHeight: Double
    let outputDirectory: String
    let onOpenSTL: () -> Void
    let onExport: () -> Void
    @Binding var livePreview: Bool
    let onPreview: () -> Void
    let onPreviewLayers: () -> Void
    let isGeneratingPreview: Bool
    let previewProgress: Double
    let isExporting: Bool
    let progress: Double
    let report: ExportReport?
    let status: String
    let onShowDiagnostics: () -> Void

    private let axes = ["+X", "-X", "+Y", "-Y", "+Z", "-Z"]
    private let formats = [("svg", "SVG"), ("png", "PNG"), ("stacked-svg", "Stacked SVG"), ("stacked-stl", "Stacked STL")]

    var body: some View {
        Form {
            DisclosureGroup("Profile", isExpanded: $profileExpanded) {
                HStack {
                    Picker("Active profile", selection: $profileController.selectedID) { ForEach(profileController.profiles) { Text($0.name).tag($0.id) } }
                    Menu { Button { profileController.create() } label: { Label("New", systemImage: "plus") }; Button { profileController.duplicate() } label: { Label("Duplicate", systemImage: "plus.square.on.square") }; Divider(); Button(role: .destructive) { profileController.deleteSelected() } label: { Label("Delete", systemImage: "trash") } } label: { Image(systemName: "ellipsis.circle") }.menuStyle(.borderlessButton).accessibilityLabel("Profile actions")
                }
                HStack { Button(action: onOpenSTL) { Label("Open STL…", systemImage: "doc.badge.plus") }; Spacer() }
            }
            Section("Preview") {
                Toggle("Live preview", isOn: $livePreview)
                HStack {
                    Button(action: onPreview) {
                        if isGeneratingPreview {
                            Label("Previewing…", systemImage: "hourglass")
                            ProgressView(value: previewProgress).controlSize(.small)
                        } else {
                            Label("Preview", systemImage: "play.fill")
                        }
                    }
                    .disabled(livePreview || isGeneratingPreview)
                    Spacer()
                    Button(action: onPreviewLayers) {
                        Label("Preview layers", systemImage: "square.stack.3d.up")
                    }
                }
            }
            DisclosureGroup("Model Orientation", isExpanded: $orientationExpanded) {
                Picker("Rotation axis", selection: profileBinding(\.axis)) { ForEach(axes, id: \.self, content: Text.init) }
                sliderRow("Fine rotation (°)", value: profileBinding(\.rotation), range: -180...180)
                sliderRow("Scale", value: profileBinding(\.scale), range: 0.1...10)
            }
            DisclosureGroup("Slicing", isExpanded: $slicingExpanded) {
                sliderRow("Layer height (mm)", value: profileBinding(\.layerHeight), range: 0.1...10)
                LabeledContent("Estimated layers", value: normalizedHeight > 0 ? "\(Int(ceil(normalizedHeight / profileController.activeProfile.layerHeight)))" : "Load a model")
            }
            DisclosureGroup("Cleanup", isExpanded: $cleanupExpanded) {
                Picker("Mode", selection: profileBinding(\.cleanupMode)) { Text("Preserve").tag("preserve"); Text("Warn").tag("warn"); Text("Apply").tag("apply") }
                threshold("Feature width (mm)", keyPath: \.featureWidth); threshold("Island area (mm²)", keyPath: \.islandArea); threshold("Hole width (mm)", keyPath: \.holeWidth); threshold("Bridge width (mm)", keyPath: \.bridgeWidth)
            }
            DisclosureGroup("Output", isExpanded: $outputExpanded) {
                Picker("Format", selection: formatBinding) {
                    ForEach(formats, id: \.0) { Text($0.1).tag($0.0) }
                }
                Stepper("DPI: \(profileController.activeProfile.outputDPI)", value: profileBinding(\.outputDPI), in: 1...2400)
                if selectedFormat == "stacked-svg" {
                    Picker("Cricut size", selection: cricutSizeBinding) {
                        Text("Cricut Normal").tag("cricut-normal")
                        Text("Cricut Large").tag("cricut-large")
                    }
                }
                if !profileController.activeProfile.outputOptions.isEmpty { Text("Additional output options are preserved:").font(.caption); ForEach(profileController.activeProfile.outputOptions.keys.sorted(), id: \.self) { key in Text("\(key): \(profileController.activeProfile.outputOptions[key] ?? "")").font(.caption.monospaced()).foregroundStyle(.secondary) } }
                VStack(alignment: .leading, spacing: 6) {
                    Button(action: onExport) { Label(isExporting ? "Exporting…" : "Export…", systemImage: "square.and.arrow.up") }.disabled(isExporting)
                    if isExporting { ProgressView(value: progress).controlSize(.small) }
                    if let report {
                        Text(outputDirectory).font(.caption).foregroundStyle(.secondary).lineLimit(1)
                        Text(status).font(.caption).foregroundStyle(.secondary)
                        Text("\(report.summary.layerCount) layer(s), \(report.summary.pageCount) page(s), \(report.summary.totalBytes) bytes")
                            .font(.caption).foregroundStyle(.secondary)
                        if !report.failures.isEmpty { Text("\(report.failures.count) file(s) failed").font(.caption).foregroundStyle(.red) }
                        if !report.summary.diagnostics.isEmpty || !report.failures.isEmpty {
                            Button(action: onShowDiagnostics) {
                                Label("Show diagnostics (\(report.summary.diagnostics.count + report.failures.count))",
                                      systemImage: report.failures.isEmpty ? "exclamationmark.triangle" : "xmark.octagon")
                            }
                            .accessibilityHint("Opens a selectable list of export warnings and errors")
                        }
                    }
                }
            }
            DisclosureGroup("Cricut", isExpanded: $cricutExpanded) {
                Picker("Packing", selection: profileBinding(\.packing)) { Text("Fixed").tag("fixed"); Text("Tight").tag("tight") }
                LabeledContent("Tile gap (mm)") {
                    NumericField("Gap (mm)", value: profileBinding(\.cricutGap))
                }
                LabeledContent("Guide inset (mm)") {
                    NumericField("Guide inset (mm)", value: profileBinding(\.guideInset))
                }
                Toggle("Show layer numbers in combined SVG", isOn: profileBinding(\.showLayerNumbers))
                TextField("Number font size (mm)", value: profileBinding(\.layerNumberFontSize), format: numberFormat)
                LabeledContent("Page estimate", value: profileController.activeProfile.outputFormat.contains("cricut") ? "Calculated on preview" : "Not applicable")
            }
        }
        .formStyle(.grouped).padding()
    }

    private func profileBinding<T>(_ keyPath: WritableKeyPath<SlicingProfile, T>) -> Binding<T> { Binding(get: { profileController.activeProfile[keyPath: keyPath] }, set: { var profile = profileController.activeProfile; profile[keyPath: keyPath] = $0; profileController.activeProfile = profile }) }
    private var formatBinding: Binding<String> { Binding(get: { selectedFormat }, set: { value in var profile = profileController.activeProfile; if value == "stacked-svg" { profile.outputFormat = profile.outputFormat == "cricut-large" ? "cricut-large" : "cricut-normal"; profile.combinedCricut = true } else { profile.outputFormat = value; profile.combinedCricut = false }; profileController.activeProfile = profile }) }
    private var selectedFormat: String { ["cricut-normal", "cricut-large", "cricut-combined"].contains(profileController.activeProfile.outputFormat) ? "stacked-svg" : profileController.activeProfile.outputFormat }
    private var cricutSizeBinding: Binding<String> { Binding(get: { profileController.activeProfile.outputFormat == "cricut-large" ? "cricut-large" : "cricut-normal" }, set: { value in var profile = profileController.activeProfile; profile.outputFormat = value; profile.combinedCricut = true; profileController.activeProfile = profile }) }
    private func profileBinding(_ keyPath: WritableKeyPath<SlicingProfile, Double?>, default defaultValue: Double) -> Binding<Double> { Binding(get: { profileController.activeProfile[keyPath: keyPath] ?? defaultValue }, set: { var profile = profileController.activeProfile; profile[keyPath: keyPath] = $0 > 0 ? $0 : nil; profileController.activeProfile = profile }) }
    private func threshold(_ title: String, keyPath: WritableKeyPath<SlicingProfile, Double>) -> some View { NumericField(title, value: profileBinding(keyPath)) }
    private func sliderRow(_ title: String, value: Binding<Double>, range: ClosedRange<Double>) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack { Text(title); Spacer(); NumericField(value: value).frame(width: 100) }
            HStack(spacing: 0) {
                Slider(value: value, in: range)
                    .frame(maxWidth: .infinity)
            }
            .frame(maxWidth: .infinity)
            .layoutPriority(1)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.horizontal, 0)
        .fixedSize(horizontal: false, vertical: true)
        .gridCellColumns(2)
    }
    private var numberFormat: FloatingPointFormatStyle<Double> { .number.locale(Locale(identifier: "C")).precision(.fractionLength(0...2)) }
}

private struct NumericField: View {
    let title: String?
    @Binding var value: Double
    @State private var text = ""

    init(_ title: String? = nil, value: Binding<Double>) {
        self.title = title
        self._value = value
    }

    var body: some View {
        TextField("", text: $text)
            .textFieldStyle(.roundedBorder)
            .font(.body.monospacedDigit())
            .multilineTextAlignment(.trailing)
            .accessibilityLabel(title ?? "Numeric value")
            .onAppear { text = renderedValue(value) }
            .onChange(of: value) { _, newValue in
                if Double(text) != newValue { text = renderedValue(newValue) }
            }
            .onChange(of: text) { _, newText in
                if let parsed = Double(newText), parsed.isFinite { value = parsed }
            }
            .onSubmit { text = renderedValue(value) }
    }

    private func renderedValue(_ value: Double) -> String {
        value.formatted(.number.locale(Locale(identifier: "en_US_POSIX")).precision(.fractionLength(2)))
    }
}
