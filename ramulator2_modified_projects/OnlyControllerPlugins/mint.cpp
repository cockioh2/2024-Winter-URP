#include <vector>
#include <unordered_map>
#include <limits>
#include <random>
#include <queue>
#include <utility>


#include "base/base.h"
#include "dram_controller/bh_controller.h"
#include "dram_controller/plugin.h"
#include "dram_controller/impl/plugin/prac/prac.h"
#include "addr_mapper/impl/rit.h"
#include "global_flags.h"

int global_vrr_flag = 0;
int global_vrr_t_flag = 0;

namespace Ramulator {

class MINT : public IControllerPlugin, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IControllerPlugin, MINT, "MINT", "MINT, Minimalist In-DRAM Tracker")

    private: 
        IDRAM* m_dram = nullptr;
        int CAN = 0;
        int SAN = -1;
        AddrVec_t SAR;
        bool SAR_valid = false;
        Clk_t m_clk = 0;
        int m_tREFI = -1;
        int   m_seed;  

        std::mt19937 m_generator;
        std::uniform_int_distribution<int> dist; 

        int m_VRR_req_id = -1;
        int m_VRR_T_req_id = -1; 
        int m_bank_level = -1;
        int m_row_level = -1;
        int m_max_access = -1;
        std::queue<std::pair<AddrVec_t, bool>> DMQ; 
        int refresh_postponement = -1;

    public:
        void init() override {
            m_max_access = param<int>("access_max_count").default_val(73);
            m_seed = param<int>("seed").desc("Seed for the RNG").default_val(123);
            m_generator = std::mt19937(m_seed);
            dist = std::uniform_int_distribution<int>(1,m_max_access + 1);
            SAN = dist(m_generator); 
            
        };

        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override { 
            m_ctrl = cast_parent<IBHDRAMController>(); 
            m_dram = m_ctrl->m_dram;
            m_tREFI = m_dram->m_timing_vals("nREFI");
            if (m_tREFI <= 0) {
                throw std::runtime_error("Invalid tREFI value: " + std::to_string(m_tREFI));
            }
            if (!m_dram->m_commands.contains("VRR")) {
            throw ConfigurationError("MINT requires a DRAM implementation with Victim-Row-Refresh (VRR) support!");
            }
            m_VRR_req_id = m_dram->m_requests("victim-row-refresh");

            m_VRR_T_req_id = m_dram->m_requests("vrr-transitive");
            
            m_bank_level = m_dram->m_levels("bank");
            m_row_level = m_dram->m_levels("row");

        };

        void update(bool request_found, ReqBuffer::iterator& req_it) override { 
            m_clk++;
            if (request_found) {
                if (
                    m_dram->m_command_meta(req_it->command).is_opening && 
                    m_dram->m_command_scopes(req_it->command) == m_row_level
                ) {
                    CAN++;
                    if (CAN == SAN){
                        SAR = req_it->addr_vec;
                        SAR_valid = true;

                    }
                }
            }
            if (m_clk % m_tREFI == 0){
                refresh();
                if (refresh_postponement == 1){
                    DMQ_push();
                }
            }
            
            
        };

        void refresh(){
            while (!DMQ.empty()) {
                auto [addr, is_vrr_t] = DMQ.front();
                DMQ.pop(); 
                if (is_vrr_t) {
                    Request vrr_t_req(addr, m_VRR_T_req_id);
                    m_ctrl->priority_send(vrr_t_req);
                } else {
                    Request vrr_req(addr, m_VRR_req_id);
                    m_ctrl->priority_send(vrr_req);
                }
            }
            if (SAR_valid){
                if (SAN == m_max_access + 1){
                    Request vrr_t_req(SAR, m_VRR_T_req_id); 
                    m_ctrl->priority_send(vrr_t_req);
                    global_vrr_t_flag ++;
                }
                else{
                    Request vrr_req(SAR, m_VRR_req_id); 
                    m_ctrl->priority_send(vrr_req);
                    global_vrr_flag ++;
                }
            }
            SAN = dist(m_generator);
            CAN = 0;
            if (SAN != m_max_access + 1) {
            SAR_valid = false;
            }
        }       
        
        void DMQ_push(){
            DMQ.push({SAR, SAR_valid && (SAN == m_max_access + 1)});
        }


}; // End of the class MINT

}