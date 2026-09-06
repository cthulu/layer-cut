import Foundation

// MARK: - C ABI Bindings

@_silgen_name("slicer_load_stl")
func layer_cut_load_stl(_ path: UnsafePointer<CChar>) -> UnsafeMutableRawPointer?

@_silgen_name("slicer_config_create")
func layer_cut_config_create() -> UnsafeMutableRawPointer?

@_silgen_name("slicer_config_set_layer_height")
func layer_cut_config_set_layer_height(_ config: UnsafeMutableRawPointer, _ mm: CDouble) -> Int32

@_silgen_name("slicer_config_set_canvas")
func layer_cut_config_set_canvas(_ config: UnsafeMutableRawPointer, _ width: CDouble, _ height: CDouble) -> Int32

@_silgen_name("slicer_config_set_format")
func layer_cut_config_set_format(_ config: UnsafeMutableRawPointer, _ format: Int32) -> Int32

@_silgen_name("slicer_config_set_dpi")
func layer_cut_config_set_dpi(_ config: UnsafeMutableRawPointer, _ dpi: Int32) -> Int32

@_silgen_name("slicer_config_set_cricut_gap")
func layer_cut_config_set_cricut_gap(_ config: UnsafeMutableRawPointer, _ gap: CDouble) -> Int32

@_silgen_name("slicer_config_set_cricut_guide_inset")
func layer_cut_config_set_cricut_guide_inset(_ config: UnsafeMutableRawPointer, _ inset: CDouble) -> Int32

@_silgen_name("slicer_config_set_cleanup_mode")
func layer_cut_config_set_cleanup_mode(_ config: UnsafeMutableRawPointer, _ mode: Int32) -> Int32

@_silgen_name("slicer_config_set_cleanup_thresholds")
func layer_cut_config_set_cleanup_thresholds(_ config: UnsafeMutableRawPointer, _ featureWidth: CDouble, _ islandArea: CDouble, _ holeWidth: CDouble, _ bridgeWidth: CDouble) -> Int32

@_silgen_name("slicer_slice")
func layer_cut_slice(_ mesh: UnsafeMutableRawPointer, _ config: UnsafeMutableRawPointer) -> UnsafeMutableRawPointer?

@_silgen_name("slicer_result_layer_count")
func layer_cut_result_layer_count(_ result: UnsafeMutableRawPointer) -> Int32

@_silgen_name("slicer_result_page_count")
func layer_cut_result_page_count(_ result: UnsafeMutableRawPointer) -> Int32

@_silgen_name("slicer_result_warning_count")
func layer_cut_result_warning_count(_ result: UnsafeMutableRawPointer) -> Int32

@_silgen_name("slicer_result_layer_svg")
func layer_cut_result_layer_svg(_ result: UnsafeMutableRawPointer, _ index: Int32) -> UnsafePointer<CChar>?

@_silgen_name("slicer_result_layer_png")
func layer_cut_result_layer_png(_ result: UnsafeMutableRawPointer, _ index: Int32, _ size: UnsafeMutablePointer<Int>) -> UnsafePointer<UInt8>?

@_silgen_name("slicer_result_page_cut_svg")
func layer_cut_result_page_cut_svg(_ result: UnsafeMutableRawPointer, _ page: Int32) -> UnsafePointer<CChar>?

@_silgen_name("slicer_result_page_guide_svg")
func layer_cut_result_page_guide_svg(_ result: UnsafeMutableRawPointer, _ page: Int32) -> UnsafePointer<CChar>?

@_silgen_name("slicer_result_page_layer_start")
func layer_cut_result_page_layer_start(_ result: UnsafeMutableRawPointer, _ page: Int32) -> Int32

@_silgen_name("slicer_result_page_layer_count")
func layer_cut_result_page_layer_count(_ result: UnsafeMutableRawPointer, _ page: Int32) -> Int32

@_silgen_name("slicer_result_page_path_count")
func layer_cut_result_page_path_count(_ result: UnsafeMutableRawPointer, _ page: Int32) -> Int32

@_silgen_name("slicer_result_warning")
func layer_cut_result_warning(_ result: UnsafeMutableRawPointer, _ index: Int32) -> UnsafePointer<CChar>?

@_silgen_name("slicer_last_error")
func layer_cut_last_error() -> UnsafePointer<CChar>?

@_silgen_name("slicer_free_mesh")
func layer_cut_free_mesh(_ mesh: UnsafeMutableRawPointer?)

@_silgen_name("slicer_free_config")
func layer_cut_free_config(_ config: UnsafeMutableRawPointer?)

@_silgen_name("slicer_free_result")
func layer_cut_free_result(_ result: UnsafeMutableRawPointer?)

// MARK: - Swift Bridge

enum EngineError: Error {
    case loadFailed(String)
    case sliceFailed(String)
    case invalidIndex(String)
    case noData(String)
    case lastError(String)
}

final class MeshHandle {
    let raw: UnsafeMutableRawPointer
    init(raw: UnsafeMutableRawPointer) {
        self.raw = raw
    }
    deinit {
        layer_cut_free_mesh(raw)
    }
}

final class ConfigHandle {
    let raw: UnsafeMutableRawPointer
    init(raw: UnsafeMutableRawPointer) {
        self.raw = raw
    }
    deinit {
        layer_cut_free_config(raw)
    }
}

final class SliceResultHandle {
    let raw: UnsafeMutableRawPointer
    init(raw: UnsafeMutableRawPointer) {
        self.raw = raw
    }
    deinit {
        layer_cut_free_result(raw)
    }
}

enum OutputFormat {
    case svg
    case png
    case cricutNormal
    case cricutLarge

    var rawValue: Int32 {
        switch self {
        case .svg: return 0
        case .png: return 1
        case .cricutNormal: return 2
        case .cricutLarge: return 3
        }
    }
}

enum CleanupMode {
    case preserve
    case warn
    case apply

    var rawValue: Int32 {
        switch self {
        case .preserve: return 0
        case .warn: return 1
        case .apply: return 2
        }
    }
}

func loadStl(path: String) throws -> MeshHandle {
    let cPath = (path as NSString).utf8String
    guard let cPath = cPath else {
        throw EngineError.loadFailed("Invalid path")
    }
    let raw = layer_cut_load_stl(cPath)
    guard let raw = raw else {
        throw EngineError.loadFailed(lastErrorMessage() ?? "Unknown load error")
    }
    return MeshHandle(raw: raw)
}

func createConfig() throws -> ConfigHandle {
    guard let raw = layer_cut_config_create() else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Could not create configuration")
    }
    return ConfigHandle(raw: raw)
}

func configSetLayerHeight(_ config: ConfigHandle, mm: Double) throws {
    guard layer_cut_config_set_layer_height(config.raw, mm) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set layer height")
    }
}

func configSetCanvas(_ config: ConfigHandle, width: Double, height: Double) throws {
    guard layer_cut_config_set_canvas(config.raw, width, height) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set canvas")
    }
}

func configSetFormat(_ config: ConfigHandle, _ format: OutputFormat) throws {
    guard layer_cut_config_set_format(config.raw, format.rawValue) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set format")
    }
}

func configSetDpi(_ config: ConfigHandle, dpi: Int32) throws {
    guard layer_cut_config_set_dpi(config.raw, dpi) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set DPI")
    }
}

func configSetCricutGap(_ config: ConfigHandle, gap: Double) throws {
    guard layer_cut_config_set_cricut_gap(config.raw, gap) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set Cricut gap")
    }
}

func configSetCricutGuideInset(_ config: ConfigHandle, inset: Double) throws {
    guard layer_cut_config_set_cricut_guide_inset(config.raw, inset) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set Cricut guide inset")
    }
}

func configSetCleanupMode(_ config: ConfigHandle, _ mode: CleanupMode) throws {
    guard layer_cut_config_set_cleanup_mode(config.raw, mode.rawValue) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set cleanup mode")
    }
}

func configSetCleanupThresholds(_ config: ConfigHandle, featureWidth: Double, islandArea: Double, holeWidth: Double, bridgeWidth: Double) throws {
    guard layer_cut_config_set_cleanup_thresholds(config.raw, featureWidth, islandArea, holeWidth, bridgeWidth) != 0 else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Failed to set cleanup thresholds")
    }
}

func slice(mesh: MeshHandle, config: ConfigHandle) throws -> SliceResultHandle {
    let raw = layer_cut_slice(mesh.raw, config.raw)
    guard let raw = raw else {
        throw EngineError.sliceFailed(lastErrorMessage() ?? "Unknown slice error")
    }
    return SliceResultHandle(raw: raw)
}

func resultLayerCount(_ result: SliceResultHandle) -> Int32 {
    layer_cut_result_layer_count(result.raw)
}

func resultPageCount(_ result: SliceResultHandle) -> Int32 {
    layer_cut_result_page_count(result.raw)
}

func resultWarningCount(_ result: SliceResultHandle) -> Int32 {
    layer_cut_result_warning_count(result.raw)
}

func resultLayerSvg(_ result: SliceResultHandle, index: Int32) -> String? {
    let ptr = layer_cut_result_layer_svg(result.raw, index)
    guard let ptr = ptr else { return nil }
    return String(cString: ptr)
}

func resultLayerPng(_ result: SliceResultHandle, index: Int32) -> Data? {
    var size: Int = 0
    let ptr = layer_cut_result_layer_png(result.raw, index, &size)
    guard let ptr = ptr, size > 0 else { return nil }
    return Data(bytes: ptr, count: size)
}

func resultPageCutSvg(_ result: SliceResultHandle, page: Int32) -> String? {
    let ptr = layer_cut_result_page_cut_svg(result.raw, page)
    guard let ptr = ptr else { return nil }
    return String(cString: ptr)
}

func resultPageGuideSvg(_ result: SliceResultHandle, page: Int32) -> String? {
    let ptr = layer_cut_result_page_guide_svg(result.raw, page)
    guard let ptr = ptr else { return nil }
    return String(cString: ptr)
}

func resultPageLayerStart(_ result: SliceResultHandle, page: Int32) -> Int32 {
    layer_cut_result_page_layer_start(result.raw, page)
}

func resultPageLayerCount(_ result: SliceResultHandle, page: Int32) -> Int32 {
    layer_cut_result_page_layer_count(result.raw, page)
}

func resultPagePathCount(_ result: SliceResultHandle, page: Int32) -> Int32 {
    layer_cut_result_page_path_count(result.raw, page)
}

func resultWarning(_ result: SliceResultHandle, index: Int32) -> String? {
    let ptr = layer_cut_result_warning(result.raw, index)
    guard let ptr = ptr else { return nil }
    return String(cString: ptr)
}

func lastErrorMessage() -> String? {
    let ptr = layer_cut_last_error()
    guard let ptr = ptr else { return nil }
    let msg = String(cString: ptr)
    return msg.isEmpty ? nil : msg
}
