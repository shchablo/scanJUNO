--------------------------------------------------------------------------------
-- Copyright [2016] [Shchablo Konstantin]
--------------------------------------------------------------------------------
-- Information.
-- Company: JINR PMTLab
-- Author: Shchablo Konstantin
-- Email: ShchabloKV@gmail.com
-- Tel: 8-906-796-76-53 (russia)
---------------------------------------------------------------------------------

--	-------------------------------------------------------
--	CLOCK_DIVIDER.vhd
--	-------------------------------------------------------
--	design by Shchablo Konstantin (ShchabloKV@gmail.com)
--		Version: 0.0 Test
--		Software: Quartus II 9.1(x32)
--						 last change: 18.04.2016
--	-------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity CLOCK_DIVIDER is
	GENERIC (DELAY: integer := 16 );
	PORT 
	( 
		CLK : in  STD_LOGIC;
		CLK_OUT : out  STD_LOGIC := '0'
	);
end CLOCK_DIVIDER;

architecture Behavioral of CLOCK_DIVIDER is
	
begin	
	
	process(CLK)
	
		variable counter : integer range 0 to DELAY := 0;
		
		begin
		
			if(CLK'event AND CLK='1') then
				counter := counter + 1;
				if counter = DELAY / 2 then
					CLK_OUT <= '0';
				elsif counter = DELAY then
					counter := 0;
					CLK_OUT <= '1';
				end if;
				
			end if;
	end process;
	
end Behavioral;
