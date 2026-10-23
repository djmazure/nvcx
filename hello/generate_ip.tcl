#!/usr/bin/env -S vivado -mode batch -source

# Any part will do for a behavioural simulation
set parts [get_parts -filter {ARCHITECTURE == zynquplus}]
if {[llength $parts] == 0} { set parts [get_parts] }
create_project -force hello [file join [pwd] vivado] -part [lindex $parts 0]
set_property target_language VHDL [current_project]

# Create a DDS Compiler IP from the installed Vivado...
create_ip -name dds_compiler \
	-vendor xilinx.com \
	-library ip \
	-version 6.0 \
	-module_name hello_dds

set_property -dict [list                                \
    CONFIG.Parameter_Entry   {Hardware_Parameters}      \
    CONFIG.PartsPresent      {Phase_Generator_and_SIN_COS_LUT} \
    CONFIG.Phase_Increment   {Fixed}                    \
    CONFIG.Phase_Width       {16}                       \
    CONFIG.PINC1             {0000010000000000}         \
    CONFIG.Output_Width      {8}                        \
    CONFIG.Output_Selection  {Sine_and_Cosine}          \
    CONFIG.Noise_Shaping     {None}                     \
    CONFIG.Has_ARESETn       {true}                     \
    CONFIG.Has_Phase_Out     {false}                    \
] [get_ips hello_dds]

# ...and export its simulator collateral
generate_target simulation [get_ips]
export_ip_user_files -of_objects [get_ips] -no_script -force
export_simulation -force \
	-simulator xsim \
	-absolute_path \
	-directory [file join [pwd] export_sim] \
	-of_objects [get_ips hello_dds]
