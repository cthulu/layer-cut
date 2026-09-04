file(REMOVE_RECURSE "${OUTPUT_DIR}")
execute_process(
  COMMAND "${CLI}" "${INPUT}" --output-dir "${OUTPUT_DIR}" --layer-height 0.2
          --format svg
  RESULT_VARIABLE result
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "CLI failed (${result})\nstdout: ${stdout}\nstderr: ${stderr}")
endif()

file(GLOB files "${OUTPUT_DIR}/layer_*.svg")
list(LENGTH files file_count)
if(NOT file_count EQUAL 533)
  message(FATAL_ERROR "Expected 533 SVG files, got ${file_count}")
endif()
list(GET files 0 first_file)
file(READ "${first_file}" first_svg)
if(NOT first_svg MATCHES "fill-rule=\"evenodd\"")
  message(FATAL_ERROR "SVG is missing the even-odd fill rule")
endif()
if(NOT first_svg MATCHES "viewBox=")
  message(FATAL_ERROR "SVG is missing a viewBox")
endif()
