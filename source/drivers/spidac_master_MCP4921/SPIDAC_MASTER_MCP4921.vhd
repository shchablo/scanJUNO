--------------------------------------------------------------------------------
-- Copyright [2016] [Shchablo Konstantin]
--
-- Licensed under the Apache License, Version 2.0 (the "License"); 
-- you may not use this file except in compliance with the License. 
-- You may obtain a copy of the License at
--
-- http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, 
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
-- either express or implied. 
-- See the License for the specific language governing permissions and
-- limitations under the License.
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Information.
-- Company: JINR PMTLab
-- Author: Shchablo Konstantin
-- Email: ShchabloKV@gmail.com
-- Tel: 8-906-796-76-53 (russia)
---------------------------------------------------------------------------------

--	-------------------------------------------------------
--	SPI_MASTER_DAC_MCP4921.vhd
--	-------------------------------------------------------
--	design by Shchablo Konstantin (ShchabloKV@gmail.com)
--		Version: 0.0 Test
--		Software: Quartus II 9.1(x32)
--						 last change: 18.04.2016
--	-------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity SPIDAC_MASTER_MCP4921 is
	port
	(
		clk: IN std_logic; -- 40 MHz 
		data: IN std_logic_vector (11 downto 0); -- 12 bit data
		load: IN std_logic; -- load for read data
		
		SCK : OUT std_logic; -- 20 MHz 
		nCS: OUT std_logic; -- chip select
		nLDAC: OUT std_logic;
		SDI: OUT std_logic
	);
end SPIDAC_MASTER_MCP4921;

architecture Behavioral of SPIDAC_MASTER_MCP4921 is

	type STATE_TYPE is (INITIALIZATION, IDLE, REQUEST_DATA, INDEX_DECREMENT, REQUEST_LOAD, SEND);
	signal state: STATE_TYPE := INITIALIZATION;
	signal prev_data : std_logic_vector (11 downto 0) := (others => 'X');
	signal write_buffer : std_logic_vector (15 downto 0) := (others => '0');
	signal	SDI_iv :		std_logic;
  signal	request:		std_logic;
	
	constant DELAY:integer := 2;
	constant IGNORE:std_logic := '0'; -- 0:use, 1:ignore
	constant BUFFERED:std_logic := '0'; -- 0:unbuffered, 1:buffered
	constant GAIN:std_logic := '1'; -- 0:2X, 1:1X
	constant ACT:std_logic := '1'; -- 0:shutdown, 1:active

begin
	
	clk2SCK : entity work.ClOCK_DIVIDER(Behavioral) 
					 generic map(DELAY => DELAY)
					 port map (clk, SCK);	
	
	process(clk)
		
		VARIABLE 	bit_ix: INTEGER RANGE 0 to 32;
		begin		
				
		if rising_edge(clk) then
			case state is
				WHEN INITIALIZATION =>
					bit_ix := 16;
					nCS <= '1';
					nLDAC <= '0';
					SDI_iv <= '0';
					request <= '0';
					state <= IDLE;
				
				WHEN IDLE =>
					bit_ix := 16;
					nCS <= '1';
					nLDAC <= '0';
					SDI_iv <= '0';
					if data /= prev_data then
						state <= REQUEST_DATA;
					elsif(request = '1' AND load = '1') then
						state <= REQUEST_LOAD;
					end if;
					
				when REQUEST_DATA =>
					prev_data <= data;
					request <= '1';
					state <= IDLE;
					
					
				when REQUEST_LOAD =>
					request <= '0';
					nCS <= '0';
					nLDAC <= '1';
					write_buffer <= (IGNORE & BUFFERED & GAIN & ACT & prev_data);
						state <= INDEX_DECREMENT;
						
				when INDEX_DECREMENT =>
						if(bit_ix = 0) then
							state <= IDLE;
						else
							bit_ix := bit_ix - 1;
							state <= SEND;
						end if;
								
				when SEND =>
						SDI_iv <= write_buffer(bit_ix);
						state <= INDEX_DECREMENT;
				
				when others =>
					state <= idle;			
											
			end case;
			
			
		end if;
	end process;

SDI <= SDI_iv;
	
end Behavioral;