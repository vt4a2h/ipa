find_package(Catch2 3 CONFIG REQUIRED)

function(ipa_add_test test_name)
    string(REPLACE "_" "-" test_target_name "${test_name}")
    set(test_target_name "test-${test_target_name}")

    add_executable(${test_target_name})
    target_sources(${test_target_name} PRIVATE
        tst_${test_name}.cpp
    )

    target_compile_features(${test_target_name} PRIVATE cxx_std_23)

    set_target_properties(${test_target_name} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_link_libraries(${test_target_name} PRIVATE
        ipa::ipa

        Catch2::Catch2
        Catch2::Catch2WithMain
    )

    include(Catch)
    catch_discover_tests(${test_target_name})

    set(ipa_test_target_name ${test_target_name} PARENT_SCOPE)
endfunction()

add_subdirectory(ipa)
