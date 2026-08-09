if(NOT DEFINED src OR src STREQUAL "")
  message(FATAL_ERROR "VerifyStrictBundleSignature.cmake requires -Dsrc=<bundle path>")
endif()

if(NOT APPLE)
  return()
endif()

if(NOT EXISTS "${src}")
  message(FATAL_ERROR "Bundle does not exist: ${src}")
endif()

find_program(SENDBLOOM_CODESIGN_EXECUTABLE codesign REQUIRED)

function(sendbloom_verify_strict_signature out_result out_output)
  execute_process(
    COMMAND "${SENDBLOOM_CODESIGN_EXECUTABLE}" --verify --deep --strict --verbose=2 "${src}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)
  set(${out_result} "${result}" PARENT_SCOPE)
  set(${out_output} "${stdout}${stderr}" PARENT_SCOPE)
endfunction()

sendbloom_verify_strict_signature(initial_result initial_output)

if(NOT initial_result EQUAL 0)
  if(NOT SENDBLOOM_ADHOC_SIGN_IF_NEEDED)
    message(FATAL_ERROR
      "Strict code-sign verification failed for ${src}:\n${initial_output}")
  endif()

  # JUCE writes generated resources, including moduleinfo.json, after its
  # initial signing hook.  Finalise the outer local/ad-hoc bundle only after
  # that materialisation has completed, then verify the final bytes strictly.
  execute_process(
    COMMAND "${SENDBLOOM_CODESIGN_EXECUTABLE}" --force --sign - "${src}"
    RESULT_VARIABLE signing_result
    OUTPUT_VARIABLE signing_stdout
    ERROR_VARIABLE signing_stderr)

  if(NOT signing_result EQUAL 0)
    message(FATAL_ERROR
      "Unable to apply the final ad-hoc bundle signature to ${src}:\n"
      "${signing_stdout}${signing_stderr}")
  endif()
endif()

sendbloom_verify_strict_signature(final_result final_output)

if(NOT final_result EQUAL 0)
  message(FATAL_ERROR
    "Strict code-sign verification failed for ${src}:\n${final_output}")
endif()

message(STATUS "SENDBLOOM strict bundle signature verified: ${src}")
