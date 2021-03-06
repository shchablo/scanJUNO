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
		SDI: OUT std_logic;
		
		start_count: OUT std_logic;
		
		reset: IN std_logic
	);
end SPIDAC_MASTER_MCP4921;

architecture Behavioral of SPIDAC_MASTER_MCP4921 is

	type STATE_TYPE is (INITIALIZATION, IDLE, REQUEST_DATA, INDEX_DECREMENT, REQUEST_LOAD, SEND, ENDSEND);
	signal state: STATE_TYPE := INITIALIZATION;
	signal prev_data : std_logic_vector (11 downto 0) := (others => 'X');
	signal write_buffer : std_logic_vector (15 downto 0) := (others => '0');
	signal	SDI_iv :		std_logic;
	signal	SCK_iv :		std_logic;
	signal	request:		std_logic;
	signal	even:		std_logic;
	signal	start_count_iv :		std_logic;
	
	constant DELAY:integer := 2;
	constant IGNORE:std_logic := '0'; -- 0:use, 1:ignore
	constant BUFFERED:std_logic := '0'; -- 0:unbuffered, 1:buffered
	constant GAIN:std_logic := '1'; -- 0:2X, 1:1X
	constant ACT:std_logic := '1'; -- 0:shutdown, 1:active

begin
	
	clk2SCK : entity work.ClOCK_DIVIDER(Behavioral) 
					 generic map(DELAY => DELAY)
					 port map (clk, SCK_iv);	
	
	process(clk)
		
		VARIABLE 	bit_ix: INTEGER RANGE 0 to 32;
		begin		
				
		if rising_edge(clk) then
			
			if(even = '0') then
				even <= '1';
			elsif(even = '1') then
				even <= '0';
			end if;
			
			case state is
				WHEN INITIALIZATION =>
					if(reset = '1') then
						state <= INITIALIZATION;
					end if;
					bit_ix := 16;
					nCS <= '1';
					nLDAC <= '1';
					start_count_iv <= '0';
					SDI_iv <= '0';
					request <= '0';
					state <= IDLE;
				
				WHEN IDLE =>
					if(reset = '1') then
						state <= INITIALIZATION;
					end if;
					bit_ix := 16;
					nCS <= '1';
					nLDAC <= '1';
					start_count_iv <= '0';
					SDI_iv <= '0';
					--if data /= prev_data then
						--state <= REQUEST_DATA;
					--elsif(request = '1' AND load = '1' AND even = '1') then
					if(load = '1' AND even = '1') then
						state <= REQUEST_LOAD;
					end if;
					
				when REQUEST_DATA =>
					if(reset = '1') then
						state <= INITIALIZATION;
					end if;
					prev_data <= data;
					request <= '1';
					state <= IDLE;
					
					
				when REQUEST_LOAD =>
					if(reset = '1') then
						state <= INITIALIZATION;
					end if;
					request <= '0';
					--write_buffer <= (IGNORE & BUFFERED & GAIN & ACT & prev_data);
					write_buffer <= (IGNORE & BUFFERED & GAIN & ACT & data);
						state <= INDEX_DECREMENT;
						
				when INDEX_DECREMENT =>
					if(reset = '1') then
						state <= INITIALIZATION;
					end if;
						if(bit_ix = 20) then
							bit_ix := 0;
							SDI_iv <= '0';
							state <= ENDSEND;
						else
							bit_ix := bit_ix - 1;
							state <= SEND;
						end if;
								
				when SEND =>
					if(reset = '1') then
						state <= INITIALIZATION;
					end if;
						if(bit_ix = 0) then
							SDI_iv <= write_buffer(bit_ix);
							bit_ix := 20;
						elsif(bit_ix = 20) then
							--nCS <= '1';
							state <= INDEX_DECREMENT;
						else 
							nCS <= '0';
							SDI_iv <= write_buffer(bit_ix);
							state <= INDEX_DECREMENT;
						end if;
				
				when ENDSEND =>
					if(reset = '1') then
						state <= INITIALIZATION;
					end if;
						if(bit_ix = 0) then
							nCS <= '1';
							nLDAC <= '0';
							start_count_iv <= '1';
							bit_ix := bit_ix + 1;
						elsif(bit_ix = 16) then
							nLDAC <= '1';
							start_count_iv <= '0';
							bit_ix := 16;
							state <= IDLE;
						else
							bit_ix := bit_ix + 1;
						end if;
				
				when others =>
					state <= IDLE;			
											
			end case;
			
			
		end if;
	end process;

SDI <= SDI_iv;
SCK <= SCK_iv;
start_count <= start_count_iv;
	
end Behavioral;
