execute_process(
  COMMAND "${CLI}" --help
  RESULT_VARIABLE help_result
  OUTPUT_VARIABLE help_stdout
  ERROR_VARIABLE help_stderr
)
if(NOT help_result EQUAL 0 OR NOT help_stdout MATCHES "--cutting-axis" OR
   NOT help_stdout MATCHES "--rotate-x" OR NOT help_stdout MATCHES "--rotate-y" OR
   NOT help_stdout MATCHES "--rotate-z" OR NOT help_stdout MATCHES "--scale" OR
   NOT help_stdout MATCHES "scale.*axis orientation.*Rz.*Ry.*Rx")
  message(FATAL_ERROR "Transform options or order missing from CLI help\n${help_stdout}\n${help_stderr}")
endif()

execute_process(
  COMMAND "${CLI}" --rotate-x 181 "${INPUT}"
  RESULT_VARIABLE invalid_result
  OUTPUT_VARIABLE invalid_stdout
  ERROR_VARIABLE invalid_stderr
)
if(invalid_result EQUAL 0 OR NOT invalid_stderr MATCHES "Error")
  message(FATAL_ERROR "Invalid transform was accepted\nstdout: ${invalid_stdout}\nstderr: ${invalid_stderr}")
endif()
if(invalid_stderr MATCHES "Cannot open STL")
  message(FATAL_ERROR "CLI loaded the STL before rejecting the transform\n${invalid_stderr}")
endif()

set(identity "${CMAKE_CURRENT_BINARY_DIR}/identity.stl")
set(transformed "${CMAKE_CURRENT_BINARY_DIR}/transformed.stl")
set(repeat "${CMAKE_CURRENT_BINARY_DIR}/repeat.stl")
set(identity_dir "${CMAKE_CURRENT_BINARY_DIR}/identity-svg")
set(transformed_dir "${CMAKE_CURRENT_BINARY_DIR}/transformed-svg")
set(repeat_dir "${CMAKE_CURRENT_BINARY_DIR}/repeat-svg")
execute_process(COMMAND "${CLI}" --cutting-axis +Z --rotate-x 0 --rotate-y 0 --rotate-z 0 --scale 1
                         --output-dir "${identity_dir}" --stacked-stl "${identity}" "${INPUT}"
                RESULT_VARIABLE identity_result)
execute_process(COMMAND "${CLI}" --cutting-axis +Z --rotate-x 31 --rotate-y -17 --rotate-z 43 --scale 1.25
                         --output-dir "${transformed_dir}" --stacked-stl "${transformed}" "${INPUT}"
                RESULT_VARIABLE transformed_result)
execute_process(COMMAND "${CLI}" --cutting-axis +Z --rotate-x 31 --rotate-y -17 --rotate-z 43 --scale 1.25
                         --output-dir "${repeat_dir}" --stacked-stl "${repeat}" "${INPUT}"
                RESULT_VARIABLE repeat_result)
if(NOT identity_result EQUAL 0 OR NOT transformed_result EQUAL 0 OR NOT repeat_result EQUAL 0)
  message(FATAL_ERROR "CLI transform export failed")
endif()
file(SHA256 "${identity}" identity_hash)
file(SHA256 "${transformed}" transformed_hash)
file(SHA256 "${repeat}" repeat_hash)
if(identity_hash STREQUAL transformed_hash OR NOT transformed_hash STREQUAL repeat_hash)
  message(FATAL_ERROR "Transform output was not changed or is not byte-stable")
endif()
file(GLOB identity_svg "${identity_dir}/*.svg")
file(GLOB transformed_svg "${transformed_dir}/*.svg")
file(GLOB repeat_svg "${repeat_dir}/*.svg")
list(LENGTH identity_svg identity_count)
list(LENGTH transformed_svg transformed_count)
list(LENGTH repeat_svg repeat_count)
if(identity_count EQUAL 0 OR transformed_count EQUAL 0 OR repeat_count EQUAL 0)
  message(FATAL_ERROR "CLI did not produce SVG layers")
endif()
list(GET identity_svg 0 identity_svg_file)
list(GET transformed_svg 0 transformed_svg_file)
list(GET repeat_svg 0 repeat_svg_file)
file(SHA256 "${identity_svg_file}" identity_svg_hash)
file(SHA256 "${transformed_svg_file}" transformed_svg_hash)
file(SHA256 "${repeat_svg_file}" repeat_svg_hash)
if(identity_svg_hash STREQUAL transformed_svg_hash OR NOT transformed_svg_hash STREQUAL repeat_svg_hash)
  message(FATAL_ERROR "Transform SVG output was not changed or is not byte-stable")
endif()
