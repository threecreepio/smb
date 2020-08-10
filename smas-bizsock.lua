local sock = 0
event.onframeend(function()
	local sockvalue = bit.lshift(memory.readbyte(0x78), 16)
		+ bit.lshift(memory.readbyte(0x219), 8)
		+ memory.readbyte(0x401)
		+ (bit.rshift(0xFF - memory.readbyte(0x3B8), 2) * 0x280)
	if bit.band(memory.readbyte(memory.readbyte(0x787)), 3) == 1 then
		sock = sockvalue
	end
	gui.drawText(25, 10, string.format('%06X', sock), null, "black", 12)
end)
