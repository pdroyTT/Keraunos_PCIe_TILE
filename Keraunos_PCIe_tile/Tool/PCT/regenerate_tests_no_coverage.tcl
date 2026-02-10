#!/usr/bin/env pctsh
#
# Regenerate unit tests WITHOUT coverage instrumentation
#

# Start fresh project
new_project

# Import the model
source [file normalize [file dirname [info script]]]/Keranous_pcie_tile_import.tcl

# Regenerate unit tests WITHOUT coverage
puts "Regenerating unit tests without coverage..."
::pct::create_unit_tests_for_component \
    SYSTEM_LIBRARY Keranous_pcie_tile/keraunos::pcie::KeraunosPcieTile \
    [file normalize [file dirname [info script]]]/../../Tests/Unittests \
    -no_coverage

puts "Unit tests regenerated successfully without coverage instrumentation"
