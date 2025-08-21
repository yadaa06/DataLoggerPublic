idf_component_register(SRCS "speaker_task.cpp" "speaker.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES driver webserver esp_http_server)


find_package(Python3 REQUIRED)

function(generate_header_from_wav FILE_BASE_NAME)
    set(WAV_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_BASE_NAME}.wav)
    set(HEADER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_BASE_NAME}.h)

    add_custom_command(
        OUTPUT ${HEADER_FILE}
        COMMAND ${CMAKE_COMMAND} -E echo "Generating ${FILE_BASE_NAME}.h from ${FILE_BASE_NAME}.wav"
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/audio_data_generator.py ${WAV_FILE} ${HEADER_FILE}
        DEPENDS ${WAV_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/audio_data_generator.py
        COMMENT "Running audio_data_generator.py to generate ${FILE_BASE_NAME}.h"
    )

    set_source_files_properties(${HEADER_FILE} PROPERTIES GENERATED TRUE)
    target_sources(${COMPONENT_LIB} PRIVATE ${HEADER_FILE})
endfunction()

generate_header_from_wav(reading_taken)
generate_header_from_wav(power_on)
generate_header_from_wav(power_off)