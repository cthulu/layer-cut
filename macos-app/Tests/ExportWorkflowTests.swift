import Foundation

@main
struct ExportWorkflowTests {
    static func main() {
        precondition(ExportFileNaming.layer(index: 2, extension: "svg") == "layer_002.svg")
        let page = CricutPageOutput(index: 3, cutSVG: "", guideSVG: "", combinedSVG: "",
                                    layerStart: 7, layerCount: 12, pathCount: 1)
        precondition(ExportFileNaming.cricutPrefix(page: page) == "page_003_layers_007-018")
        precondition(ExportFileNaming.layer(index: 1000, extension: "png") == "layer_1000.png")
    }
}
