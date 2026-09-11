import SwiftUI
import WebKit

struct LayerPreviewInline: View {
    let output: SliceOutput?
    @Binding var selection: Int
    let layerCount: Int
    let isGenerating: Bool
    let progress: Double
    let error: String?
    let onRetry: () -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Layer")
                NumericField("Layer number", value: layerNumber)
                    .frame(width: 74)
                Text("of \(max(layerCount, 0))").foregroundStyle(.secondary)
            }
            Slider(value: layerSelection, in: 0...Double(max(layerCount - 1, 0)))
                .disabled(layerCount == 0 || isGenerating)
                .help("Select the layer to display.")

            ZStack {
                RoundedRectangle(cornerRadius: 6).fill(.black.opacity(0.08))
                if isGenerating {
                    VStack(spacing: 8) {
                        ProgressView(value: progress)
                        Label("Generating layer preview…", systemImage: "hourglass")
                            .font(.caption)
                    }
                    .padding()
                } else if let error {
                    VStack(spacing: 8) {
                        Label("Preview unavailable", systemImage: "exclamationmark.triangle")
                        Text(error).font(.caption).foregroundStyle(.secondary).multilineTextAlignment(.center)
                        Button("Retry", action: onRetry)
                    }
                    .padding()
                } else if let svg = selectedSVG {
                    SVGPreview(svg: svg, zoom: 1)
                        .padding(8)
                } else if output == nil {
                    ContentUnavailableView("No layer preview", systemImage: "square.dashed",
                                           description: Text("Enable the preview after loading an STL model."))
                } else {
                    ContentUnavailableView("No layer data", systemImage: "square.dashed")
                }
            }
            .frame(height: 280)
            .accessibilityElement(children: .contain)
            .accessibilityLabel("SVG preview for layer \(selection + 1)")
        }
    }

    private var selectedSVG: String? {
        guard let output, selection >= 0, selection < output.layers.count else { return nil }
        return output.layers[selection].previewSVG ?? output.layers[selection].svg
    }

    private var layerSelection: Binding<Double> {
        Binding(get: { Double(selection) }, set: { selection = min(max(Int($0.rounded()), 0), max(layerCount - 1, 0)) })
    }

    private var layerNumber: Binding<Double> {
        Binding(get: { Double(selection + 1) }, set: { selection = min(max(Int($0.rounded()) - 1, 0), max(layerCount - 1, 0)) })
    }
}

private struct SVGPreview: NSViewRepresentable {
    let svg: String
    let zoom: Double

    func makeNSView(context: Context) -> WKWebView {
        let view = WKWebView()
        view.setValue(false, forKey: "drawsBackground")
        return view
    }

    func updateNSView(_ view: WKWebView, context: Context) {
        let scale = Int(zoom * 100)
        view.loadHTMLString("<html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><style>html,body{width:100%;height:100%;margin:0;background:transparent;overflow:hidden}.preview{width:100%;height:100%;display:flex;align-items:center;justify-content:center}.preview svg{width:\(scale)%;height:\(scale)%;max-width:100%;max-height:100%;display:block}</style></head><body><div class=\"preview\">\(svg)</div></body></html>", baseURL: nil)
    }
}
