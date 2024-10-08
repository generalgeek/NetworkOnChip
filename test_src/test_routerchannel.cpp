#include <systemc.h>
#include <memory>
#include <string>
#include <vector>
#include "flit.h"
#include "router.h"
#include "routerchannel.h"

using namespace sc_core;
void ConnectEmptyRChannel(std::vector<Router*> routers, std::vector<RouterChannel*> routerchannels);
class PE_Sender : public sc_module {
    Flit flit_;  // 31~25: source id, 24~16: dest id, 15~0: data
   public:
    sc_in<bool> clk;
    sc_port<IRouterChannel> ichannel_;  // 输入通道
    sc_port<IRouterChannel> ochannel_;  // 输出通道

    SC_CTOR(PE_Sender) : flit_(flit_t(0x00010001), 0) {
        SC_THREAD(process);
        sensitive << clk.pos();
        // dont_initialize();
    }

    void process() {
        while (true) {
            wait();  // 等待时钟上升沿
            flit_.setData(flit_.getData() + 1);
            std::cout << sc_time_stamp() << " " << this->name() << " start process." << std::endl;
            wait(2, SC_NS);               // 模拟处理耗时:2ns
            ochannel_->sendFlit(&flit_);  // send header flit
            std::cout << sc_time_stamp() << " " << this->name() << " send flit: " << std::hex << "0x" << flit_.getData() << std::endl;
            for (sc_uint<16> i = 0; i < flit_.getData().range(15, 0); i++) {
                ochannel_->sendFlit(&flit_);  // send other flits
                std::cout << sc_time_stamp() << " " << this->name() << " send flit: " << std::hex << "0x" << flit_.getData() << std::endl;
            }
        }
    }
};
class PE_Receiver : public sc_module {
    Flit* flit_;  // 31~25: source id, 24~16: dest id, 15~0: data
   public:
    sc_port<IRouterChannel> ichannel_;  // 输入通道
    sc_port<IRouterChannel> ochannel_;  // 输出通道

    SC_CTOR(PE_Receiver) {
        SC_THREAD(process);
        // dont_initialize();
    }

    void process() {
        while (true) {
            flit_ = ichannel_->receiveFlit();
            std::cout << sc_time_stamp() << " " << this->name() << " receive flit: " << std::hex << "0x" << flit_->getData() << std::endl;
        }
    }
};
/**
 *      ____               ____
 *     |r0  |*----rc0----*|r1  |
 *     |____|*----rc1----*|____|
 *      \\                  \\
 *      pe0                 pe1
 */

int sc_main(int argc, char* argv[]) {
    std::vector<Router*> routers;
    std::vector<RouterChannel*> routerchannels;
    sc_clock g_clk("g_clk", 10, SC_NS, 0.5, 0, SC_NS, false);  // 时钟周期10ns,高低电平各占一半,起始时刻为0,起始为低电平

    // 创建两个路由器r0、r1并连接输入输出通道
    Router* r0 = new Router("Router_0", 0);
    Router* r1 = new Router("Router_1", 1);
    RouterChannel* rc0 = new RouterChannel("RounterChannel_0");
    RouterChannel* rc1 = new RouterChannel("RounterChannel_1");
    r0->eastChannelOut(*rc0);
    r1->westChannelIn(*rc0);
    r0->eastChannelIn(*rc1);
    r1->westChannelOut(*rc1);
    routers.push_back(r0);
    routers.push_back(r1);
    routerchannels.push_back(rc0);
    routerchannels.push_back(rc1);

    // 往路由器r0上挂载一个PE(省略了Shell和NI), 连接输入输出通道
    RouterChannel* rc2 = new RouterChannel("RounterChannel_2");
    RouterChannel* rc3 = new RouterChannel("RounterChannel_3");
    routerchannels.push_back(rc2);
    routerchannels.push_back(rc3);
    PE_Sender pe("pe0");
    pe.clk(g_clk);
    pe.ochannel_(*rc2);
    r0->localChannelIn(*rc2);
    pe.ichannel_(*rc3);
    r0->localChannelOut(*rc3);

    // 往路由器r1上挂载一个PE(省略了Shell和NI), 连接输入输出通道
    RouterChannel* rc4 = new RouterChannel("RounterChannel_4");
    RouterChannel* rc5 = new RouterChannel("RounterChannel_5");
    routerchannels.push_back(rc4);
    routerchannels.push_back(rc5);
    PE_Receiver pe1("pe1");
    pe1.ichannel_(*rc4);
    r1->localChannelOut(*rc4);
    pe1.ochannel_(*rc5);
    r1->localChannelIn(*rc5);

    // 给路由器上没连接实际通道的端口, 连无效通道上去, 以免报错
    ConnectEmptyRChannel(routers, routerchannels);

    sc_trace_file* tf = sc_create_vcd_trace_file("trace");
    // tf->set_time_unit(1, SC_NS);
    sc_trace(tf, g_clk, "g_clk");
    sc_trace(tf, pe.clk, "pe_clk");
    sc_start(50, SC_NS);
    sc_close_vcd_trace_file(tf);

    for (auto* r : routers) {
        delete r;
        r = nullptr;
    }
    for (auto* rc : routerchannels) {
        delete rc;
        rc = nullptr;
    }
    return 0;
}

void ConnectEmptyRChannel(std::vector<Router*> routers, std::vector<RouterChannel*> routerchannels) {
    for (Router* router : routers) {
        if (router->localChannelIn.bind_count() == 0) {
            RouterChannel* rc0 = new RouterChannel(std::string(router->getName() + std::to_string(Link::Local) + "_0").c_str());
            RouterChannel* rc1 = new RouterChannel(std::string(router->getName() + std::to_string(Link::Local) + "_1").c_str());
            routerchannels.push_back(rc0);
            routerchannels.push_back(rc1);
            router->localChannelIn(*rc0);
            router->localChannelOut(*rc1);
        }
        if (router->eastChannelIn.bind_count() == 0) {
            RouterChannel* rc0 = new RouterChannel(std::string(router->getName() + std::to_string(Link::East) + "_0").c_str());
            RouterChannel* rc1 = new RouterChannel(std::string(router->getName() + std::to_string(Link::East) + "_1").c_str());
            routerchannels.push_back(rc0);
            routerchannels.push_back(rc1);
            router->eastChannelIn(*rc0);
            router->eastChannelOut(*rc1);
        }
        if (router->southChannelIn.bind_count() == 0) {
            RouterChannel* rc0 = new RouterChannel(std::string(router->getName() + std::to_string(Link::South) + "_0").c_str());
            RouterChannel* rc1 = new RouterChannel(std::string(router->getName() + std::to_string(Link::South) + "_1").c_str());
            routerchannels.push_back(rc0);
            routerchannels.push_back(rc1);
            router->southChannelIn(*rc0);
            router->southChannelOut(*rc1);
        }
        if (router->westChannelIn.bind_count() == 0) {
            RouterChannel* rc0 = new RouterChannel(std::string(router->getName() + std::to_string(Link::West) + "_0").c_str());
            RouterChannel* rc1 = new RouterChannel(std::string(router->getName() + std::to_string(Link::West) + "_1").c_str());
            routerchannels.push_back(rc0);
            routerchannels.push_back(rc1);
            router->westChannelIn(*rc0);
            router->westChannelOut(*rc1);
        }
        if (router->northChannelIn.bind_count() == 0) {
            RouterChannel* rc0 = new RouterChannel(std::string(router->getName() + std::to_string(Link::North) + "_0").c_str());
            RouterChannel* rc1 = new RouterChannel(std::string(router->getName() + std::to_string(Link::North) + "_1").c_str());
            routerchannels.push_back(rc0);
            routerchannels.push_back(rc1);
            router->northChannelIn(*rc0);
            router->northChannelOut(*rc1);
        }
    }
}
