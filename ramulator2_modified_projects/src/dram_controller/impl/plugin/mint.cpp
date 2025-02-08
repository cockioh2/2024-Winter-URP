#include <vector>
#include <unordered_map>
#include <limits>
#include <random>
#include <queue>

#include "base/base.h"
#include "dram_controller/controller.h"
#include "dram_controller/plugin.h"

namespace Ramulator {

class MINT : public IControllerPlugin, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IControllerPlugin, MINT, "MINT", "MINT, Minimalist In-DRAM Tracker")

    private: // Variables which have to be implemented
        IDRAM* m_dram = nullptr;
        int CAN = 0;
        int SAN = 0;
        AddrVec_t SAR;
        bool SAR_valid = false;
        Clk_t m_clk = 0;
        int m_tREFI = -1;

        std::mt19937 m_generator; // Random number generator
        std::uniform_int_distribution<int> dist; 

        int m_VRR_req_id = -1;
        int m_max_access = -1;
        std::queue<AddrVec_t> DMQ;  // 요청을 저장하는 큐
        const size_t max_dmq_size = 4;

    public: // Functions which have to be implemented
        // Initialization Function
        void init() override {
            // Required Variables Initialization
            m_max_access = param<int>("access_max_count").required();
            m_generator.seed(std::random_device{}());
            dist = std::uniform_int_distribution<int>(1,m_max_access);
            SAN = dist(m_generator); 
            
        };

        // Setup function
        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override { 
            m_ctrl = cast_parent<IDRAMController>(); // 부모 클래스를 가져옴
            m_dram = m_ctrl->m_dram; // DRAM을 가져옴
            m_tREFI = m_dram->m_timing_vals("nREFI");

            if (!m_dram->m_commands.contains("VRR")) {
            throw ConfigurationError("MINT requires a DRAM implementation with Victim-Row-Refresh (VRR) support!");
            }
            // Required Configurations
            m_VRR_req_id = m_dram->m_requests("victim-row-refresh");

        };

        // Update Function
        void update(bool request_found, ReqBuffer::iterator& req_it) override { 
            m_clk++;
            if (request_found) {
                if (CAN > 73){
                    DMQ.push(SAR);
                    SAN = dist(m_generator);
                    CAN = 0;
                }
                CAN++;
                if (CAN == SAN){
                    SAR = req_it->addr_vec;
                    SAR_valid = true;
                }
            }
            if (m_clk%m_tREFI == 0){
                refresh();
            }
        };

        void refresh(){
            m_VRR_req_id = m_dram->m_requests("victim-row-refresh");
            if (!DMQ.empty()){
                AddrVec_t target = DMQ.front();
                DMQ.pop();
                Request vrr_req(target, m_VRR_req_id);
                m_ctrl->priority_send(vrr_req);
            }
            else {
                if (SAR_valid){
                Request vrr_req(SAR, m_VRR_req_id); 
                m_ctrl->priority_send(vrr_req);
                SAR_valid = false;
                
                SAN = dist(m_generator);
                CAN = 0;
            }
            }
            
        }        // Additional Functions to be implemented
        


}; // End of the class MINT

}