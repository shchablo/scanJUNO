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

library ieee;
use ieee.std_logic_1164.all;

entity initializer is
	port(
		clk   : in  std_logic; -- 50 mHz
		--
		res   : out std_logic;
		n_res : out std_logic);
end initializer;

architecture behavior of initializer IS
	type state_type is (
		idle,
		cycle_Tdel,
		cycle_Tdel_x50k,
		--
		cycle_Tos_x50k,
		empty_cycle);

	signal state  : state_type;
	signal res_iv : std_logic;

begin
	state_process : process(clk)
		variable cnt_T   : integer range 0 to 65535;
		variable cnt_64k : integer range 0 to 65535;

	begin
		if (clk'event and clk = '0') then
			case state is
				----
				when idle =>
					res_iv <= '0';
					cnt_T  := 1;        -- 100 mSec

					state <= cycle_Tdel;
				----	
				when cycle_Tdel =>      -- 100 mSec delay
					if (cnt_T > 0) then
						cnt_T   := cnt_T - 1;
						cnt_64k := 50000;  -- 50000 x 20 nS = 1 mSec	

						state <= cycle_Tdel_x50k;
					else
						cnt_64k := 50000;  -- 50000 x 20 nS = 1 mSec
						res_iv  <= '1';
						state   <= cycle_Tos_x50k;
					end if;
				----
				when cycle_Tdel_x50k =>
					if (cnt_64k > 0) then
						cnt_64k := cnt_64k - 1;

						state <= cycle_Tdel_x50k;
					else
						state <= cycle_Tdel;
					end if;
				----
				when cycle_Tos_x50k =>  -- 1 mSec res pulse			
					if (cnt_64k > 0) then
						cnt_64k := cnt_64k - 1;

						state <= cycle_Tos_x50k;
					else
						res_iv <= '0';

						state <= empty_cycle;
					end if;
				-----		
				when empty_cycle =>
					state <= empty_cycle;
				-----				
				when others =>
					state <= idle;
			----
			end case;
		end if;
	end process state_process;
	--
	res   <= res_iv;
	n_res <= not (res_iv);
--
end behavior;