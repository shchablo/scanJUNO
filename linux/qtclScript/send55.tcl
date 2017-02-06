set usb [lindex [get_hardware_names] 0]
set device_name [lindex [get_device_names -hardware_name $usb] 0]
puts "*************************"
puts "programming cable:"
puts $usb

proc addr {value} {
        global device_name usb
        open_device -device_name $device_name -hardware_name $usb

        if {$value > 256} {
                return "value entered exceeds 8 bits" }

        set push_value [int2bits $value]
        set diff [expr {8 - [string length $push_value]%8}]

        if {$diff != 8} {
                set push_value [format %0${diff}d$push_value 0] }

        puts $push_value

        device_lock -timeout 1000
        device_virtual_ir_shift -instance_index 0 -ir_value 1 -no_captured_ir_value
        device_virtual_dr_shift -instance_index 0 -dr_value $push_value -length 8 -no_captured_dr_value
        device_unlock
        close_device
}

proc push {value} {
	global device_name usb
	open_device -device_name $device_name -hardware_name $usb
   
	if {$value > 256} {
		return "value entered exceeds 8 bits" }

	set push_value [int2bits $value]
	set diff [expr {8 - [string length $push_value]%8}]
    
	if {$diff != 8} {
		set push_value [format %0${diff}d$push_value 0] }

	puts $push_value

        device_lock -timeout 1000
        device_virtual_ir_shift -instance_index 0 -ir_value 2 -no_captured_ir_value
	device_virtual_dr_shift -instance_index 0 -dr_value $push_value -length 8 -no_captured_dr_value
	device_unlock
	close_device
}

proc pop {} {
	global device_name usb
	variable x
	open_device -device_name $device_name -hardware_name $usb
        device_lock -timeout 1000
        device_virtual_ir_shift -instance_index 0 -ir_value 3 -no_captured_ir_value
	set x [device_virtual_dr_shift -instance_index 0 -length 8]
	device_unlock
	close_device
	puts $x
}

proc int2bits {i} {    
	set res ""
	while {$i>0} {
		set res [expr {$i%2}]$res
		set i [expr {$i/2}]}
	if {$res==""} {set res 0}
		return $res
}

proc bin2hex bin {
	## No sanity checking is done
	array set t {
	0000 0 0001 1 0010 2 0011 3 0100 4
	0101 5 0110 6 0111 7 1000 8 1001 9
	1010 a 1011 b 1100 c 1101 d 1110 e 1111 f
	}
	set diff [expr {4-[string length $bin]%4}]
	if {$diff != 4} {
		set bin [format %0${diff}d$bin 0] }
	regsub -all .... $bin {$t(&)} hex
	return [subst $hex]
}


#puts "Config gen:"
#addr 0x38
#push 0x50
#addr 0x39
#push 0xC3
#addr 0x42
#push 0x0A
#puts "SPI:"
#addr 0x24
#push 0xAA
#addr 0x25
#push 0x00
#addr 0x23
#push 0x02

#puts "Read counter:"
#addr 0x26
#push 0x02
#addr 0x28
#pop

addr 0x38
pop
#addr 0x03
#pop
#addr 0x04
#pop
#addr 0x05
#pop
#puts "Clear:"
#addr 0x02
#push 0x00
#addr 0x03
#push 0x00
#addr 0x04
#push 0x00
#addr 0x05
#push 0x00
