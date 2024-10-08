#include <systemc.h>
#include <memory>
#include <string>
#include <vector>
#include "flit.h"
#include "router.h"
#include "routerchannel.h"

class PE_Sender : public sc_module {
   public:
    sc_in<bool> clk;
    Flit flit_;
    // sc_port<IRouterChannel> ichannel_;  // 输入通道
    sc_port<IRouterChannel> ochannel_;  // 输出通道

    SC_CTOR(PE_Sender) : flit_(flit_t(100), 0) {
        SC_THREAD(process);
        sensitive << clk.pos();
        dont_initialize();
    }

    void process() {
        while (true) {
            std::cout << sc_time_stamp() << std::endl;
            wait();  // 等待时钟上升沿
            // wait(2, SC_NS);  // 模拟处理耗时:2us
            ochannel_->sendFlit(&flit_);
            flit_.setData(flit_.getData() + 1);
        }
    }
};

int sc_main(int argc, char* argv[]) {
    std::vector<Router*> routers;
    std::vector<RouterChannel*> routerchannels;
    // sc_clock g_clk("g_clk", 10, SC_NS, 0.5, 0, SC_NS);
    sc_clock g_clk("g_clk", 10, SC_NS, 0.5);

    // 往路由器Ra上挂载一个PE(省略了Shell和NI), 连接输入输出通道
    RouterChannel* rc2 = new RouterChannel("RounterChannel_2");
    RouterChannel* rc3 = new RouterChannel("RounterChannel_3");
    routerchannels.push_back(rc2);
    routerchannels.push_back(rc3);
    PE_Sender pe("PE0");
    pe.clk(g_clk);
    pe.ochannel_(*rc2);

    sc_trace_file* tf = sc_create_vcd_trace_file("trace");
    sc_trace(tf, g_clk, "g_clk");
    sc_trace(tf, pe.clk, "pe_clk");

    sc_start(100, SC_NS);

    sc_close_vcd_trace_file(tf);
    return 0;
}