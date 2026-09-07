import SwiftUI
import WebKit

struct LayerPreviewSheet: View {
    let output: SliceOutput?
    let task: Bool
    let progress: Double
    let error: String?
    @Binding var selection: Int?
    @Binding var zoom: Double
    let onCancel: () -> Void

    var body: some View {
        VStack {
            HStack {
                Text("Layer previews").font(.title2.weight(.semibold))
                Spacer()
                Button("Close", action: onCancel)
            }
            if task {
                ProgressView(value: progress) { Text("Generating SVG layer previews…") }
                    .padding(.vertical)
            } else if let error {
                ContentUnavailableView("Preview unavailable", systemImage: "exclamationmark.triangle", description: Text(error))
            } else if let output, output.layers.isEmpty {
                ContentUnavailableView("No layers generated", systemImage: "square.dashed", description: Text("The engine returned an empty layer set."))
            } else if let output {
                LayerPreview(output: output, selection: $selection, zoom: $zoom)
            } else {
                ProgressView("Preparing preview…")
            }
        }
        .padding()
    }
}

struct LayerPreview: View {
    let output: SliceOutput
    @Binding var selection: Int?
    @Binding var zoom: Double

    private let columns = [GridItem(.adaptive(minimum: 180), spacing: 12)]

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Text("Layers").font(.headline)
                Text("\(output.layers.count) total").foregroundStyle(.secondary)
                Spacer()
                Text("Zoom").font(.caption).foregroundStyle(.secondary)
                Slider(value: $zoom, in: 0.5...2.5).frame(width: 120)
                Text("SVG").font(.caption.monospaced()).foregroundStyle(.secondary)
            }
            if !output.warnings.isEmpty {
                Label("\(output.warnings.count) warning\(output.warnings.count == 1 ? "" : "s")", systemImage: "exclamationmark.triangle.fill")
                    .foregroundStyle(.orange)
                    .help(output.warnings.joined(separator: "\n"))
            }
            ScrollView {
                LazyVGrid(columns: columns, spacing: 12) {
                    ForEach(output.layers, id: \.index) { layer in
                        LayerCard(layer: layer, page: page(for: layer.index), zoom: zoom, isSelected: selection == layer.index)
                            .onTapGesture { selection = layer.index }
                    }
                }
                .padding(.vertical, 2)
            }
        }
        .padding(12)
        .background(.bar)
    }

    private func page(for layer: Int) -> Int? {
        output.pages.first(where: { ($0.layerStart..<$0.layerStart + $0.layerCount).contains(layer) })?.index
    }
}

private struct LayerCard: View {
    let layer: LayerOutput
    let page: Int?
    let zoom: Double
    let isSelected: Bool

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            ZStack {
                RoundedRectangle(cornerRadius: 8).fill(.black.opacity(0.08))
                if let svg = layer.svg, !svg.isEmpty {
                    SVGPreview(svg: svg, zoom: zoom)
                        .padding(8)
                } else {
                    VStack(spacing: 4) {
                        Image(systemName: layer.isEmpty ? "square.dashed" : "exclamationmark.triangle")
                        Text(layer.isEmpty ? "Empty layer" : "Preview unavailable").font(.caption)
                    }.foregroundStyle(.secondary)
                }
            }
            .frame(height: 150)
            Text("Layer \(layer.index + 1)  •  Z \(layer.z, specifier: "%.2f") mm").font(.callout.weight(.medium))
            HStack(spacing: 8) {
                Text(layer.isEmpty ? "Empty" : "Cut data")
                if let page { Text("Page \(page + 1)") }
            }.font(.caption).foregroundStyle(.secondary)
        }
        .padding(8)
        .background(.background, in: RoundedRectangle(cornerRadius: 10))
        .overlay(RoundedRectangle(cornerRadius: 10).stroke(isSelected ? Color.accentColor : .clear, lineWidth: 2))
        .contentShape(Rectangle())
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
