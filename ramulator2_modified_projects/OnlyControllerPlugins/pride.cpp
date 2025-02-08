#include <vector>
#include <unordered_map>
#include <limits>
#include <random>
#include <deque>

#include "base/base.h"
#include "dram_controller/bh_controller.h"
#include "dram_controller/plugin.h"
#include "addr_mapper/impl/rit.h"
#include "dram_controller/impl/plugin/prac/prac.h"
#include "global_flags.h"


namespace Ramulator {

class PrIDE : public IControllerPlugin, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IControllerPlugin, PrIDE, "PrIDE", "PrIDE, Probabilistic In-DRAM Tracker")

    private:
        IDRAM* m_dram = nullptr;
        IBHDRAMController* m_ctrl = nullptr;
        IPRAC* m_prac = nullptr;

        Clk_t m_clk = 0;
        int m_rank_level = -1;
        int m_bank_group_level = -1;
        int m_bank_level = -1;
        int m_row_level = -1;
        int m_col_level = -1;

        int m_fifo_max_size = -1;            // 최대 FIFO 크기
        float m_insert_probability = -1;    // 삽입 확률
        bool m_is_debug = false;            // 디버그 모드
        int m_prac_threshold = -1;          // PRAC 임계값

        // FIFO Entry 구조체
        struct FifoEntry {
            Addr_t row_addr;          // Row ID
            int mitigation_level;   // 완화 수준
        };

        // [bank_group][bank] -> FIFO
        std::unordered_map<int, std::unordered_map<int, std::deque<FifoEntry>>> m_fifo_buffers;
        std::mt19937 m_generator;           // 난수 생성기
        std::uniform_real_distribution<float> m_distribution; // 삽입 확률 분포

        int m_tREFI = -1;                   // tREFI 주기
        int m_VRR_req_id = -1;              // Victim-Row Refresh 명령 ID
        int m_VRR_T_req_id = -1;            // Victim-Row Refresh for Transitive Attack 명령 ID
        int m_max_mitigation_level = -1;     // 최대 완화 수준
        int m_row_activation_count = -1;    // Row 활성화 횟수

        int vrr_ref = 0;

    public:
        // 초기화 함수
        void init() override {
            m_fifo_max_size = param<int>("fifo_max_size").required();
            m_insert_probability = param<float>("insert_probability").required();
            m_max_mitigation_level = param<int>("max_mitigation_level").default_val(3);
            m_prac_threshold = param<int>("prac_threshold").default_val(512);
            m_is_debug = param<bool>("debug").default_val(false);


            int seed = param<int>("seed").default_val(1234);
            m_generator = std::mt19937(seed);
            m_distribution = std::uniform_real_distribution<float>(0.0f, 1.0f);

            if (m_fifo_max_size <= 0) {
                throw ConfigurationError("Invalid FIFO max size ({}) for PrIDE!", m_fifo_max_size);
            }
            if (m_insert_probability <= 0.0f || m_insert_probability >= 1.0f) {
                throw ConfigurationError("Invalid insert probability ({}) for PrIDE!", m_insert_probability);
            }
        };

        // 설정 함수
        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
            m_ctrl = cast_parent<IBHDRAMController>();
            m_dram = m_ctrl->m_dram;
            m_prac = m_ctrl->get_plugin<IPRAC>();

            if (!m_dram) {
                throw std::runtime_error("DRAM instance is null. Ensure DRAM implementation is properly set up.");
            }
            if (!m_prac) {
                std::cout << "Alert: PRAC is not plugged in PrIDE. Check your configuration is properly set up." << std::endl;
            }
            m_tREFI = m_dram->m_timing_vals("nREFI");

            m_row_level = m_dram->m_levels("row");
            m_bank_level = m_dram->m_levels("bank");
            m_bank_group_level = m_dram->m_levels("bankgroup");

            int num_banks = m_dram->get_level_size("bank");
            int num_bank_groups = m_dram->get_level_size("bankgroup");

            for (int bank_group = 0; bank_group < num_bank_groups; ++bank_group) {
                for (int bank = 0; bank < num_banks; ++bank) {
                    m_fifo_buffers[bank_group][bank] = std::deque<FifoEntry>();
                }
            }

            if (!m_dram->m_requests.contains("victim-row-refresh")) {
                throw std::runtime_error("Missing 'victim-row-refresh' command in DRAM instance.");
            }
            if (!m_dram->m_requests.contains("vrr-transitive")) {
                throw std::runtime_error("Missing 'vrr-transitive' command in DRAM instance.");
            }

            m_VRR_req_id = m_dram->m_requests("victim-row-refresh");
            m_VRR_T_req_id = m_dram->m_requests("vrr-transitive");

            if (m_is_debug) {
                std::cout << "PrIDE: Initialized with FIFO max size " << m_fifo_max_size
                        << ", insert probability " << m_insert_probability
                        << ", tREFI " << m_tREFI << std::endl;
            }
            register_stat(vrr_ref).name("vrr_ref");
        };


        // 업데이트 함수 (매 클럭마다 호출됨)
        void update(bool request_found, ReqBuffer::iterator& req_it) override {
            m_clk++;

            // 활성화 요청을 확인하고 Row를 FIFO에 삽입
            if (request_found) {
                if (m_dram->m_command_meta(req_it->command).is_opening && 
                    m_dram->m_command_scopes(req_it->command) == m_row_level) {
                    int bank_id = req_it->addr_vec[m_bank_level];
                    Addr_t row_addr = req_it->addr_vec[m_row_level];
                    int bank_group_id = req_it->addr_vec[m_bank_group_level];

                    if (should_insert()) {
                        // mitigation level은 아래와 같은 방식으로 결정
                        // 처음 들어간 Row는 1
                        // 삽입될 Row가 이미 FIFO에 존재하면, mitigation level을 1 증가시켜 새로 삽입
                        int mitigation_level = 1;
                        bool is_existing = false;
                        for (auto& [bank_group_id, bank_fifos] : m_fifo_buffers) {
                            for (auto& [bank_id, fifo] : bank_fifos) {
                                for (auto& entry : fifo) {
                                    if (entry.row_addr == row_addr) {
                                        mitigation_level = entry.mitigation_level + 1;
                                        is_existing = true;
                                        break;
                                    }
                                }
                            }
                        }

                        // TODO: PRAC 추가
                        // PRAC(Per Row Activation Counting)을 통해 Mitigation이 발생하여 Evict된 Row에 대한 Mitigation level 설정
                        if (!is_existing && m_prac) {
                            m_row_activation_count = m_prac->get_activation_count(bank_id, row_addr);
                            if (m_row_activation_count > m_prac_threshold) {
                                mitigation_level += 1;
                            }
                        }

                        insert_into_fifo(bank_group_id, bank_id, row_addr, mitigation_level);
                    }
                }
            }

            // tREFI 주기에 따라 완화 로직 실행
            if (m_clk % m_tREFI == 0) {
                mitigate_rows();
            }
        };

    private:
        // 확률 기반 삽입 여부 결정
        bool should_insert() {
            return (m_distribution(m_generator) < m_insert_probability);
        }

        // FIFO에 Row 추가
        void insert_into_fifo(int bank_group_id, int bank_id, Addr_t row_addr, int level = 1) {
            if (bank_id < 0 || bank_id >= m_dram->get_level_size("bank")) {
                throw std::runtime_error("Invalid bank_id: " + std::to_string(bank_id));
            }
            if (row_addr < 0 || row_addr >= m_dram->get_level_size("row")) {
                throw std::runtime_error("Invalid row_addr: " + std::to_string(row_addr));
            }

            auto& fifo = m_fifo_buffers[bank_group_id][bank_id];

            if (fifo.size() >= m_fifo_max_size) {
                fifo.pop_front();  // FIFO가 가득 차면 가장 오래된 Row 제거
            }

            fifo.push_back({row_addr, level});
            

            if (m_is_debug) {
                std::cout << "PrIDE: Inserted row " << row_addr << ", level " << level
                        << " into FIFO for bank " << bank_id << std::endl;
            }
        }


        // FIFO에서 Row를 꺼내 완화 실행
        void mitigate_rows() {
            for (auto& [bank_group_id, bank_fifos] : m_fifo_buffers) {
                for (auto& [bank_id, fifo] : bank_fifos) {
                    if (fifo.empty()) {
                        if (m_is_debug) {
                            std::cout << "PrIDE: No rows to mitigate in bank " << bank_id << std::endl;
                        }
                        continue;
                    }

                    if (m_is_debug) {
                        std::cout << "PrIDE: Starting mitigation in bank " << bank_id
                                << ". FIFO size before mitigation: " << fifo.size() << std::endl;
                    }

                    try {
                        FifoEntry entry = fifo.front();
                        fifo.pop_front();

                        // 유효성 검증
                        if (entry.row_addr < 0 || entry.row_addr >= m_dram->get_level_size("row")) {
                            throw std::runtime_error("Invalid row_addr in FIFO: " + std::to_string(entry.row_addr));
                        }

                        issue_vrr(bank_group_id, bank_id, entry);
                    } catch (const std::exception& e) {
                        std::cerr << "PrIDE: Error during mitigation in bank " << bank_id
                                << ": " << e.what() << std::endl;
                    }

                    if (m_is_debug) {
                        std::cout << "PrIDE: Finished mitigation for bank " << bank_id
                                << ". FIFO size after mitigation: " << fifo.size() << std::endl;
                    }
                }
                // if (fifo.empty()) {
                //     if (m_is_debug) {
                //         std::cout << "PrIDE: No rows to mitigate in bank " << bank_id << std::endl;
                //     }
                //     continue;
                // }

                // if (m_is_debug) {
                //     std::cout << "PrIDE: Starting mitigation in bank " << bank_id
                //             << ". FIFO size before mitigation: " << fifo.size() << std::endl;
                // }

                // try {
                //     FifoEntry entry = fifo.front();
                //     fifo.pop_front();

                //     // 유효성 검증
                //     if (entry.row_addr < 0 || entry.row_addr >= m_dram->get_level_size("row")) {
                //         throw std::runtime_error("Invalid row_addr in FIFO: " + std::to_string(entry.row_addr));
                //     }

                //     issue_vrr(bank_id, entry);
                // } catch (const std::exception& e) {
                //     std::cerr << "PrIDE: Error during mitigation in bank " << bank_id
                //             << ": " << e.what() << std::endl;
                // }

                // if (m_is_debug) {
                //     std::cout << "PrIDE: Finished mitigation for bank " << bank_id
                //             << ". FIFO size after mitigation: " << fifo.size() << std::endl;
                // }
            }
        }


        // VRR 명령 발행 및 다단계 완화
        void issue_vrr(int bank_group_id, int bank_id, FifoEntry& entry) {

            AddrVec_t addr_vec(m_dram->m_levels.size(), 0);
            addr_vec[m_bank_level] = bank_id;
            addr_vec[m_bank_group_level] = bank_group_id;
            
            Addr_t target_row = entry.row_addr;
            addr_vec[m_row_level] = target_row;

            if (entry.mitigation_level == 1) {
                Request vrr_req(addr_vec, m_VRR_req_id);
                if (addr_vec.size() != m_dram->m_levels.size()) {
                    throw std::runtime_error("Address vector size mismatch with DRAM levels.");
                }

                try {
                    m_ctrl->priority_send(vrr_req);
                    global_vrr_flag ++;
                    vrr_ref ++;
                } catch (const std::exception& e) {
                    throw std::runtime_error("Failed to send VRR request: " + std::string(e.what()));
                }

                if (m_is_debug) {
                    std::cout << "PrIDE: Preparing VRR request with addr_vec: ";
                    for (auto val : addr_vec) {
                        std::cout << val << " ";
                    }
                    std::cout << " and VRR_req_id: " << m_VRR_req_id << std::endl;
                }
                if (m_is_debug) {
                    std::cout << "PrIDE: Preparing to send VRR request for row " << target_row
                            << " in bank " << bank_id << std::endl;
                }
            } 
            else {
                Request vrr_t_req(addr_vec, m_VRR_T_req_id);
                if (addr_vec.size() != m_dram->m_levels.size()) {
                    throw std::runtime_error("Address vector size mismatch with DRAM levels.");
                }

                try {
                    m_ctrl->priority_send(vrr_t_req);
                    global_vrr_t_flag ++;
                } catch (const std::exception& e) {
                    throw std::runtime_error("Failed to send VRR for Transitive request: " + std::string(e.what()));
                }

                if (m_is_debug) {
                    std::cout << "PrIDE: Preparing VRR for Transitive request with addr_vec: ";
                    for (auto val : addr_vec) {
                        std::cout << val << " ";
                    }
                    std::cout << " and VRR_T_req_id: " << m_VRR_T_req_id << std::endl;
                }
            }

        };
};

} // namespace Ramulator

