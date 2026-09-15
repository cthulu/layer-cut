import SwiftUI

struct ParametersPanel: View {
    @ObservedObject var profileController: ProfileController
    @Binding var transform: TransformSession
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
    @Binding var layerPreview: Bool
    @Binding var activeLayer: Int
    let layerOutput: SliceOutput?
    let layerCount: Int
    let onPreview: () -> Void
    let isGeneratingPreview: Bool
    let previewProgress: Double
    let isGeneratingLayerPreview: Bool
    let layerPreviewProgress: Double
    let previewError: String?
    let onRetryLayerPreview: () -> Void
    let isExporting: Bool
    let progress: Double
    let report: ExportReport?
    let status: String
    let onShowDiagnostics: () -> Void

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
                Toggle("Layer preview", isOn: $layerPreview)
                    .help("Generate an SVG preview of the selected layer and the next-layer guide.")
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
                }
                if layerPreview {
                    LayerPreviewInline(output: layerOutput, selection: $activeLayer,
                                       layerCount: layerCount, isGenerating: isGeneratingLayerPreview,
                                       progress: layerPreviewProgress, error: previewError,
                                       onRetry: onRetryLayerPreview)
                }
            }
            DisclosureGroup("Transform", isExpanded: $orientationExpanded) {
                Picker("Cutting axis", selection: $transform.cuttingAxis) {
                    ForEach(TransformSession.axes, id: \.self, content: Text.init)
                }
                .help("The orientation axis along which the model will be sliced.")
                sliderRow("Rotate X (deg)", value: transformBinding(\.rotateX, range: TransformSession.angleRange), range: TransformSession.angleRange)
                    .help("Rotation around the X-axis in degrees.")
                sliderRow("Rotate Y (deg)", value: transformBinding(\.rotateY, range: TransformSession.angleRange), range: TransformSession.angleRange)
                    .help("Rotation around the Y-axis in degrees.")
                sliderRow("Rotate Z (deg)", value: transformBinding(\.rotateZ, range: TransformSession.angleRange), range: TransformSession.angleRange)
                    .help("Rotation around the Z-axis in degrees.")
                sliderRow("Scale", value: transformBinding(\.scale, range: TransformSession.scaleRange), range: TransformSession.scaleRange)
                    .help("Model scale factor.")
            }
            DisclosureGroup("Slicing", isExpanded: $slicingExpanded) {
                sliderRow("Layer height (mm)", value: profileBinding(\.layerHeight, range: 0.1...10), range: 0.1...10)
                    .help("The vertical distance (thickness) of each 3D model slice.")
                LabeledContent("Estimated layers", value: estimatedLayerCountText)
                    .help("Total slices expected based on model height and slice thickness.")
            }
            DisclosureGroup("Cleanup", isExpanded: $cleanupExpanded) {
                Picker("Mode", selection: profileBinding(\.cleanupMode)) { Text("Preserve").tag("preserve"); Text("Warn").tag("warn"); Text("Apply").tag("apply") }
                    .help("Cleanup strategy: 'Preserve' keeps all geometry, 'Warn' flags thin areas, 'Apply' automatically removes features smaller than thresholds.")
                threshold("Feature width (mm)", keyPath: \.featureWidth)
                    .help("Minimum width for features to be kept in the final slice.")
                threshold("Island area (mm²)", keyPath: \.islandArea)
                    .help("Minimum area for isolated islands to be kept.")
                threshold("Hole width (mm)", keyPath: \.holeWidth)
                    .help("Minimum width for holes to be kept.")
                threshold("Bridge width (mm)", keyPath: \.bridgeWidth)
                    .help("Minimum width for structural bridges to be kept.")
            }
            DisclosureGroup("Output", isExpanded: $outputExpanded) {
                Picker("Format", selection: formatBinding) {
                    ForEach(formats, id: \.0) { Text($0.1).tag($0.0) }
                }
                .help("The file format for exported layers.")
                Stepper("DPI: \(profileController.activeProfile.outputDPI)", value: profileBinding(\.outputDPI), in: 1...2400)
                    .help("Dots Per Inch: resolution for SVG/PNG export.")
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
                    .help("Layout strategy: 'Fixed' aligns tiles in a regular grid, 'Tight' packs them more closely to save material.")
                LabeledContent("Tile gap (mm)") {
                    NumericField("Gap (mm)", value: profileBinding(\.cricutGap))
                        .help("Spacing between individual tiles on the cutting mat.")
                        .frame(width: 100)
                }
                LabeledContent("Guide inset (mm)") {
                    NumericField("Guide inset (mm)", value: profileBinding(\.guideInset))
                        .help("Margin to shrink the alignment guide, ensuring it's hidden under the physical layer.")
                        .frame(width: 100)
                }
                Toggle("Show layer numbers in combined SVG", isOn: profileBinding(\.showLayerNumbers))
                    .help("Include layer labels in the guide SVG for easier assembly.")
                LabeledContent("Number font size (mm)") {
                    NumericField("Number font size (mm)", value: profileBinding(\.layerNumberFontSize))
                        .help("Font size for layer identification numbers.")
                        .frame(width: 100)
                }
                LabeledContent("Page estimate", value: profileController.activeProfile.outputFormat.contains("cricut") ? "Calculated on preview" : "Not applicable")
            }
        }
        .formStyle(.grouped).padding()
    }

    private func profileBinding<T>(_ keyPath: WritableKeyPath<SlicingProfile, T>) -> Binding<T> { Binding(get: { profileController.activeProfile[keyPath: keyPath] }, set: { var profile = profileController.activeProfile; profile[keyPath: keyPath] = $0; profileController.activeProfile = profile }) }
    private func profileBinding(_ keyPath: WritableKeyPath<SlicingProfile, Double>, range: ClosedRange<Double>) -> Binding<Double> {
        Binding(get: { profileController.activeProfile[keyPath: keyPath] }, set: { value in
            var profile = profileController.activeProfile
            profile[keyPath: keyPath] = min(max(value.isFinite ? value : range.lowerBound, range.lowerBound), range.upperBound)
            profileController.activeProfile = profile
        })
    }
    private func transformBinding(_ keyPath: WritableKeyPath<TransformSession, Double>, range: ClosedRange<Double>) -> Binding<Double> {
        Binding(get: { transform[keyPath: keyPath] }, set: { value in
            transform[keyPath: keyPath] = min(max(value.isFinite ? value : range.lowerBound, range.lowerBound), range.upperBound)
        })
    }
    private var formatBinding: Binding<String> { Binding(get: { selectedFormat }, set: { value in var profile = profileController.activeProfile; if value == "stacked-svg" { profile.outputFormat = profile.outputFormat == "cricut-large" ? "cricut-large" : "cricut-normal"; profile.combinedCricut = true } else { profile.outputFormat = value; profile.combinedCricut = false }; profileController.activeProfile = profile }) }
    private var selectedFormat: String { ["cricut-normal", "cricut-large", "cricut-combined"].contains(profileController.activeProfile.outputFormat) ? "stacked-svg" : profileController.activeProfile.outputFormat }
    private var cricutSizeBinding: Binding<String> { Binding(get: { profileController.activeProfile.outputFormat == "cricut-large" ? "cricut-large" : "cricut-normal" }, set: { value in var profile = profileController.activeProfile; profile.outputFormat = value; profile.combinedCricut = true; profileController.activeProfile = profile }) }
    private func profileBinding(_ keyPath: WritableKeyPath<SlicingProfile, Double?>, default defaultValue: Double) -> Binding<Double> { Binding(get: { profileController.activeProfile[keyPath: keyPath] ?? defaultValue }, set: { var profile = profileController.activeProfile; profile[keyPath: keyPath] = $0 > 0 ? $0 : nil; profileController.activeProfile = profile }) }
    private func threshold(_ title: String, keyPath: WritableKeyPath<SlicingProfile, Double>) -> some View {
        LabeledContent(title) {
            NumericField(value: profileBinding(keyPath))
                .frame(width: 100)
        }
    }
    private var estimatedLayerCountText: String {
        let layerHeight = profileController.activeProfile.layerHeight
        guard normalizedHeight > 0, layerHeight.isFinite, layerHeight > 0 else { return "Load a model" }
        return "\(Int(ceil(normalizedHeight / layerHeight)))"
    }
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

struct NumericField: View {
    let title: String?
    @Binding var value: Double
    let fractionDigits: Int
    @FocusState private var isFocused: Bool
    @State private var text = ""

    init(_ title: String? = nil, value: Binding<Double>, fractionDigits: Int = 2) {
        self.title = title
        self._value = value
        self.fractionDigits = fractionDigits
    }

    var body: some View {
        TextField("", text: $text)
            .textFieldStyle(.roundedBorder)
            .font(.body.monospacedDigit())
            .multilineTextAlignment(.trailing)
            .accessibilityLabel(title ?? "Numeric value")
            .focused($isFocused)
            .onAppear { text = renderedValue(value) }
            .onChange(of: value) { _, newValue in
                if !isFocused, Double(text) != newValue { text = renderedValue(newValue) }
            }
            .onChange(of: text) { _, newText in
                if let parsed = Double(newText), parsed.isFinite { value = parsed }
            }
            .onSubmit { text = renderedValue(value) }
    }

    private func renderedValue(_ value: Double) -> String {
        value.formatted(.number.locale(Locale(identifier: "en_US_POSIX")).precision(.fractionLength(fractionDigits)))
    }
}
