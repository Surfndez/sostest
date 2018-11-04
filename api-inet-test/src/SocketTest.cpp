﻿#include <sapi/var.hpp>
#include <sapi/sys.hpp>
#include <sapi/chrono.hpp>
#include <string.h>
#include "SocketTest.hpp"
#define MAX_PACKET_SIZE 1512
SocketTest::SocketTest() : Test("SocketTest"){}
static void rand_string_value(u16 size,String & string);
bool SocketTest::execute_class_api_case(){

    if( !execute_socket_address_info_case() ){
		print_case_failed("socket address info case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
	}
    if( !execute_socket_address_case() ){
        print_case_failed("socket address info case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
    }

    family = SocketAddressInfo::FAMILY_NONE;
    if(!execute_socket_case()){
        print_case_failed("socket case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
    }
    family = SocketAddressInfo::FAMILY_INET;
    if(!execute_socket_case()){
		print_case_failed("socket case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
	}
    family = SocketAddressInfo::FAMILY_INET6;
    if(!execute_socket_case()){
        print_case_failed("socket case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
    }
    family = SocketAddressInfo::FAMILY_INET;
    if(!execute_socket_case_udp()){
        print_case_failed("udp socket case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
    }
    family = SocketAddressInfo::FAMILY_INET6;
    if(!execute_socket_case_udp()){
        print_case_failed("udp socket case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
    }
    family = SocketAddressInfo::FAMILY_INET;
    if(!execute_socket_case_raw()){
        print_case_failed("raw socket case failed");
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
    }

    return false;
}
/*@brief start thread for listen and connect to them if needed
 * */
bool SocketTest::execute_socket_case(){

	//start the listener
	Thread thread;

	if( thread.create(listen_on_localhost_thread_function, this) < 0 ){
		print_case_failed("Failed to create listener thread");
        return false;
	}

    Timer::wait_milliseconds(100);
/*  start tcp connection handing   */
    SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_STREAM,\
                                   SocketAddressInfo::PROTOCOL_TCP,AI_PASSIVE);

    Vector<SocketAddressInfo> list = address_info.fetch_node("localhost");

	if( list.count() == 0 ){
		print_case_failed("Failed to fetch node");
        return false;
	}

	Socket local_host_socket;
	//SocketAddress localhost_address(SocketAddressIpv4(0, 8080));
	SocketAddress localhost_address(list.at(0), 8080);
    if((!localhost_address.is_ipv6() && (family == SocketAddressInfo::FAMILY_INET6))||
       (!localhost_address.is_ipv4() && (family == SocketAddressInfo::FAMILY_INET))){
        print_case_failed("ip version mismatch");
        return false;

    }

	if( local_host_socket.create(localhost_address) < 0){
		print_case_failed("Failed to create client socket");
        return false;
	}

	if( local_host_socket.connect(localhost_address) < 0 ){
		print_case_failed("Failed to connect to socket");
        return false;
	}

	String test("Testing");
	Data reply(256);

	if( local_host_socket.write(test) < 0 ){
		print_case_failed("Failed to write client socket (%d, %d)", test.size(), local_host_socket.error_number());
		perror("failed to write");
        return false;
	}

	reply.fill(0);
	if( local_host_socket.read(reply) < 0 ){
		print_case_failed("Failed to read client socket");
        return false;
	}

	print_case_message("read '%s' from socket", reply.to_char());

	if( test != reply.to_char() ){
		print_case_failed("did not get an echo on localhost");
	}

	local_host_socket.close();
/*  end tcp connection  */
#if !defined __link
    if( thread.is_running() ){//need to use on embedded

#endif
    {
/*      start udp connection handing
 *      use two sockets for write and read message
*/
        udp_server_listening = 0 ;
        SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_DGRAM,\
                                       SocketAddressInfo::PROTOCOL_UDP,AI_PASSIVE);
        list.free();
        list.clear();   //need to use before?
        list = address_info.fetch_node("localhost");
        u16 udp_port_client = 5002;
        u16 udp_port_server = 5003;
        Socket local_host_socket_udp_client;
        Socket local_host_socket_udp_server;

        SocketAddress localhost_udp_address_client(list.at(0), udp_port_client);
        SocketAddress localhost_udp_address_server(list.at(0), udp_port_server);
        if(localhost_udp_address_client.port()!=udp_port_client){
            print_case_failed("port mismatch");
            return false;
        }
        if(localhost_udp_address_server.port()!=udp_port_server){
            print_case_failed("port mismatch");
            return false;
        }
        if((family == SocketAddressInfo::FAMILY_INET) &&
                !localhost_udp_address_client.is_ipv4()){
            print_case_failed("ip version mismatch");
            return false;
        }
        if(local_host_socket_udp_client.create(localhost_udp_address_client)!=0){
            print_case_failed("create error socket");
            return false;
        }
        if(local_host_socket_udp_server.create(localhost_udp_address_server)!=0){
            print_case_failed("create error socket");
            return false;
        }

        //for udp not need it but? without write_to use connect before
        if( local_host_socket_udp_client.connect(localhost_udp_address_client) < 0 ){
            print_case_failed("Failed to connect to socket");
            return false;
        }
        Timer::wait_milliseconds(100);
        int len_writed;
        len_writed = local_host_socket_udp_client.write(test);
        if( len_writed < 0 ){
            print_case_failed("Failed to write client socket (%d, %d)", test.size(), local_host_socket_udp_client.error_number());
            perror("failed to write");
            return false;
        }
  /*      local_host_socket_udp_server.clear_error_number();
        local_host_socket_udp_server << SocketOption().reuse_address() << SocketOption().reuse_port();
*/
        local_host_socket_udp_client.close();
        reply.fill(0);

        if( local_host_socket_udp_server.bind_and_listen(localhost_udp_address_server) < 0 ){
            print_case_failed("Failed to bind to localhost (%d)", local_host_socket_udp_server.error_number());
            return 0;
        }
        if( local_host_socket_udp_server.read(reply) < 0 ){
            print_case_failed("Failed to read client socket");
            return false;
        }

        print_case_message("read '%s' from udp socket ", reply.to_char());

        if( test != reply.to_char() ){
            print_case_failed("did not get an echo on localhost");
        }

        local_host_socket_udp_server.close();
    }
#if !defined __link
    }else{
        print_case_failed("thread is stoped");
    }
#endif

/*  end udp connection handing    */

    return true;
}
/* @brief recv -> reply
 * use global option for inet family type
 * */
void * SocketTest::listen_on_localhost(){
    SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_STREAM,\
                                   SocketAddressInfo::PROTOCOL_TCP,AI_PASSIVE);

    Vector<SocketAddressInfo> list = address_info.fetch_node("localhost");

	if( list.count() == 0 ){
		print_case_failed("Failed to fetch node");
		return 0;
	}

	Socket local_host_listen_socket;
	SocketAddress localhost_address(list.at(0), 8080);
    if(family == SocketAddressInfo::FAMILY_INET){
        localhost_address = SocketAddressIpv4(0, 8080);
    }
	print_case_message("create socket at %s", localhost_address.address_to_string().str());
	if( local_host_listen_socket.create(localhost_address) < 0 ){
		print_case_failed("failed to create socket");
		return 0;
	}
	//listen on port 80 of localhost
	local_host_listen_socket.clear_error_number();
	local_host_listen_socket << SocketOption().reuse_address() << SocketOption().reuse_port();

	if( local_host_listen_socket.error_number() != 0 ){
		print_case_failed("failed to set socket options (%d)", local_host_listen_socket.error_number());
	}

	//connect to port 80 of localhost
	if( local_host_listen_socket.bind_and_listen(localhost_address) < 0 ){
		print_case_failed("Failed to bind to localhost (%d)", local_host_listen_socket.error_number());
		return 0;
	}
	print_case_message("Listening on localhost:%d", localhost_address.port());
	//now accept -- this will block until a request arrives
	SocketAddress accepted_address;
	Socket local_host_session_socket = local_host_listen_socket.accept(accepted_address);

	print_case_message("Accepted connection from %s:%d", accepted_address.address_to_string().str(), accepted_address.port());

	Data incoming(256);
	local_host_session_socket.read(incoming);
	local_host_session_socket.write(incoming);

    if(local_host_session_socket.close()<0){
        print_case_failed("failed to close socket");
        return 0;
    }

	if( local_host_listen_socket.close() < 0 ){
		print_case_failed("failed to close socket");
		return 0;
	}
    {
        /*  udp type
         * */
        SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_DGRAM,\
                                       SocketAddressInfo::PROTOCOL_UDP,AI_PASSIVE);
        list.free();
        list.clear();   //need to use before?
        list = address_info.fetch_node("localhost");
        u16 udp_port_client = 5003;
        u16 udp_port_server = 5002;
        SocketAddress localhost_udp_address_client(list.at(0), udp_port_client);
        Socket local_host_listen_udp_socket_client;
        SocketAddress localhost_udp_address_server(list.at(0), udp_port_server);
        Socket local_host_listen_udp_socket_server;

        if( local_host_listen_udp_socket_server.create(localhost_udp_address_server) < 0 ){
            print_case_failed("failed to create socket");
            return 0;
        }
        if( local_host_listen_udp_socket_server.bind_and_listen(localhost_udp_address_server) < 0 ){
            print_case_failed("Failed to bind to localhost (%d)", local_host_listen_udp_socket_server.error_number());
            return 0;
        }
        int len_handl;
        incoming.fill(0);
        len_handl = local_host_listen_udp_socket_server.read(incoming);

        if( local_host_listen_udp_socket_server.close() < 0 ){
            print_case_failed("failed to close socket");
            return 0;
        }
        Timer::wait_milliseconds(100);
        if( local_host_listen_udp_socket_client.create(localhost_udp_address_client) < 0 ){
            print_case_failed("failed to create socket");
            return 0;
        }
        //for udp not need it but? without write_to use connect before
        if( local_host_listen_udp_socket_client.connect(localhost_udp_address_client) < 0 ){
            print_case_failed("Failed to connect to socket");
            return 0;
        }
        len_handl = local_host_listen_udp_socket_client.write(incoming);
        if( len_handl < 0 ){
            print_case_failed("Failed to write client socket (%d, %d)", len_handl, local_host_listen_udp_socket_client.error_number());
            perror("failed to write");
            return 0;
        }
        if( local_host_listen_udp_socket_client.close() < 0 ){
            print_case_failed("failed to close socket");
            return 0;
        }
    }

	return 0;
}
/*@brief test address info section (getaddrinfo)
 * use:
 * set_flags() - flags(), set_family()-family(),set_type() - type(),
 * set_protocol() - protocol,
 * SocketAddressInfo,fetch_node(),fetch(),fetch_service()
 *
 * */
bool SocketTest::execute_socket_address_info_case(){
    bool result;
	SocketAddressInfo address_info;
    result = true;
	print_case_message("get list from stratifylabs.co");
    Vector<SocketAddressInfo> list = address_info.fetch_node("stratifylabs.co");

	print_case_message("got %d entries", list.count());
	for(u32 i=0; i < list.count(); i++){
		open_case(String().format("entry-%d", i));
		SocketAddress address(list.at(i), 80);
		if( address.family() == SocketAddressInfo::FAMILY_INET ){
			print_case_message("family is ipv4");
		} else {
			print_case_message("family is other:%d", address.family());
		}

		if( address.type() == SocketAddressInfo::TYPE_STREAM ){
			print_case_message("type is stream");
		} else {
			print_case_message("type is other:%d", address.type());
		}

		if( address.protocol() == SocketAddressInfo::PROTOCOL_TCP ){
			print_case_message("protocol is tcp");
		} else {
			print_case_message("protocol is other:%d", address.protocol());
		}

		print_case_message("address: %s", address.address_to_string().str());
		print_case_message("port is %d", address.port());
		close_case();
	}
    list.free();
    list.clear();
    list = address_info.fetch("stratifylabs.co","http");
    print_case_message("got http %d entries", list.count());
    if(list.count()==0){
        print_case_failed("get address http failed ");
        result = false;
    }
    list.free();
    list.clear();
    list = address_info.fetch_service("http");
    print_case_message("got only service %d entries", list.count());
    if(list.count()==0){
        print_case_failed("dont have self http service ");
        result = false;
    }
    SocketAddressInfo address_info_dos(SocketAddressInfo::FAMILY_INET6, SocketAddressInfo::TYPE_DGRAM, SocketAddressInfo::PROTOCOL_UDP, AI_PASSIVE);
    address_info_dos.set_flags(AI_PASSIVE);
    address_info_dos.set_family(SocketAddressInfo::FAMILY_NONE);
    address_info_dos.set_type(SocketAddressInfo::TYPE_STREAM);
    address_info_dos.set_protocol(SocketAddressInfo::PROTOCOL_TCP);
    if(address_info_dos.flags()!=AI_PASSIVE||
       address_info_dos.family()!=SocketAddressInfo::FAMILY_NONE||
       address_info_dos.type()!=SocketAddressInfo::TYPE_STREAM||
       address_info_dos.protocol()!=SocketAddressInfo::PROTOCOL_TCP){
        print_case_failed("don't set some property");
        result = false;
    }
    list.free();
    list.clear();
    list = address_info_dos.fetch_node("localhost");
    if(list.count()==0){
        print_case_failed("dont have self addr info ");
        result = false;
    }else{
        print_case_message("local host stream tcp list count %d",list.count());
    }

    SocketAddressInfo address_info_tres(SocketAddressInfo::FAMILY_INET, SocketAddressInfo::TYPE_DGRAM, SocketAddressInfo::PROTOCOL_UDP, AI_PASSIVE);
    if(address_info_tres.flags()!=AI_PASSIVE||
       address_info_tres.family()!=SocketAddressInfo::FAMILY_INET||
       address_info_tres.type()!=SocketAddressInfo::TYPE_DGRAM||
       address_info_tres.protocol()!=SocketAddressInfo::PROTOCOL_UDP){
        print_case_failed("don't set some property");
        result = false;
    }
    list.free();
    list.clear();
    list = address_info_tres.fetch_node("localhost");
    if(list.count()==0){
        print_case_failed("dont have self addr info");
        result = false;
    }else{
        print_case_message("local host stream udp list count %d",list.count());
    }
    return result;
}
/*@brief test socket address section (getaddrinfo)
 * use:
 *
 * */
bool SocketTest::execute_socket_address_case(){
    bool result;
    uint16_t port = 8080;
    in_addr_t IP_ADDRESS = 0;
    result = true;
    SocketAddressInfo address_info_ipv6(SocketAddressInfo::FAMILY_INET6,SocketAddressInfo::TYPE_STREAM,\
                                   SocketAddressInfo::PROTOCOL_TCP,0);
    SocketAddressInfo address_info_ipv4(SocketAddressInfo::FAMILY_INET,SocketAddressInfo::TYPE_STREAM,\
                                   SocketAddressInfo::PROTOCOL_TCP,0);

    SocketAddressIpv4 ipv4_addrees(IP_ADDRESS, port);
    sockaddr_in ipv4_in;
    sockaddr_in6 ipv6_in;

    ipv4_in.sin_addr.s_addr = IP_ADDRESS;
    ipv4_in.sin_port = port;
    for(u8 i = 0;i<16;i++){
        ipv6_in.sin6_addr.s6_addr[i] = IP_ADDRESS;
    }
    ipv6_in.sin6_port = port;
    SocketAddress socket_address_zero;
    SocketAddress socket_address_uno(ipv4_addrees);
    SocketAddress socket_address_info_ipv4(address_info_ipv4,port);
    SocketAddress socket_address_info_ipv6(address_info_ipv6,port);
    SocketAddress socket_address_ipv4(ipv4_in);
    SocketAddress socket_address_ipv6(ipv6_in);
    if( socket_address_uno.port()!=port){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv6.port()!=port){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv4.port()!=port){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_ipv4.port()!=port){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();

    }
    if(socket_address_ipv6.port()!=port){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    socket_address_zero.set_port(port);
    if(socket_address_zero.port()!=port){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }

    if(socket_address_uno.address_ipv4()!=IP_ADDRESS||
       socket_address_ipv4.address_ipv4()!=IP_ADDRESS){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv4.length() != sizeof(sockaddr_in)){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv6.length() != sizeof(sockaddr_in6)){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    socket_address_zero.set_port(port);
    if(socket_address_info_ipv6.type()!=SocketAddressInfo::TYPE_STREAM){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv4.type()!=SocketAddressInfo::TYPE_STREAM){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv6.protocol()!=SocketAddressInfo::PROTOCOL_TCP){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv4.protocol()!=SocketAddressInfo::PROTOCOL_TCP){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    socket_address_info_ipv4.set_protocol(SocketAddressInfo::PROTOCOL_UDP);
    if(socket_address_info_ipv4.protocol()!=SocketAddressInfo::PROTOCOL_UDP){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    socket_address_info_ipv4.set_type(SocketAddressInfo::TYPE_DGRAM);
    if(socket_address_info_ipv4.type()!=SocketAddressInfo::TYPE_DGRAM){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv4.family()!=SocketAddressInfo::FAMILY_INET){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(socket_address_info_ipv6.family()!=SocketAddressInfo::FAMILY_INET6){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }

    if(!socket_address_info_ipv4.is_ipv4()){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    if(!socket_address_info_ipv6.is_ipv6()){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        result = case_result();
    }
    return result;
}

bool SocketTest::execute_socket_option_case(){
    bool result;
    result = true;
    return result;
}
#define case_listen_port 5004
#define thread_listen_port 5005
#define thread_listen_port_tcp 5006

#define case_listen_port_str "5004"
#define thread_listen_port_str "5005"
#define thread_listen_port_str_tcp "5006"
/* @brief udp socket case
 * listen port - 5004
 * conection to port - 5005
 * */
bool SocketTest::execute_socket_case_udp(){
    /*      start udp connection handing
     *      use two sockets for write and read message
    */
    Thread thread;
    if( thread.create(listen_on_localhost_thread_function_udp, this) < 0 ){
        print_case_failed("Failed to create listener thread");
        return false;
    }
    Timer::wait_milliseconds(100);
    SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_DGRAM,\
                                   SocketAddressInfo::PROTOCOL_UDP,AI_PASSIVE);
    Vector<SocketAddressInfo> list = address_info.fetch("localhost",thread_listen_port_str);
    if(list.size()<=0){
        return false;
    }
    Socket local_host_socket_udp_server;
    Socket local_host_socket_udp_client;
    //address connect to
    SocketAddress localhost_udp_address_client(list.at(0), thread_listen_port);
    list.free();
    list.clear();
    list = address_info.fetch("localhost",case_listen_port_str);
    if(list.size()<=0){
        return false;
    }
    //address for listening
    SocketAddress localhost_udp_address_server(list.at(0), case_listen_port);
    //socket with sending address info
    if(local_host_socket_udp_client.create(localhost_udp_address_client)!=0){
        print_case_failed("create error socket");
        return false;
    }

    //socket with reading address info
    if(local_host_socket_udp_server.create(localhost_udp_address_server)!=0){
        print_case_failed("create error socket");
        return false;
    }
    String test("Testing");
    Data reply(256);
    int len_writed;
    Timer::wait_milliseconds(100);
    len_writed = local_host_socket_udp_client.write(test.c_str(),test.len(),localhost_udp_address_client);
    if( len_writed != (int)test.len() ){
        print_case_failed("Failed to write client socket %d",len_writed);
        return false;
    }
    reply.fill(0);
    if( local_host_socket_udp_server.bind_and_listen(localhost_udp_address_server) < 0 ){
        print_case_failed("Failed to bind to localhost (%d)");
        return 0;
    }
    int len;
    struct sockaddr ai_addr;
    socklen_t ai_addrlen;
    if(family == SocketAddressInfo::FAMILY_NONE||
       family == SocketAddressInfo::FAMILY_INET){
        ai_addrlen = sizeof(sockaddr_in);
    }else{
        ai_addrlen = sizeof(sockaddr_in6);
    }
//read one int read(void * buf, int nbyte, struct sockaddr * ai_addr,socklen_t * ai_addrlen) const;
    len = local_host_socket_udp_server.read(reply.data(),reply.size(),&ai_addr,&ai_addrlen);
    if( len < 0 ){
        print_case_failed("Failed to read client socket %d",len);
        return false;
    }

    u16 port;
    port = ai_addr.sa_data[1]<<8 | ai_addr.sa_data[2];
    print_case_message("case recv from %d.%d.%d.%d:%d ", ai_addr.sa_data[2],ai_addr.sa_data[3],\
            ai_addr.sa_data[4],ai_addr.sa_data[5],port);
    print_case_message("read '%s' from udp socket ", reply.to_char());
    if( memcmp(test.c_str(), reply.to_char(), test.length()) != 0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        print_case_failed("%s:%s",test.c_str(),reply.to_char());
    }
//read two    int read(var::Data & data, SocketAddress & address);
    SocketAddress socket_address_two(address_info);
    socket_address_two.set_protocol(SocketAddressInfo::PROTOCOL_UDP);
    len = local_host_socket_udp_server.read(reply,socket_address_two);
    if( len < 0 ){
        print_case_failed("Failed to read client socket %d",len);
        return false;
    }

    String ip_address(socket_address_two.address_to_string());
    test.to_upper();//change value
    if( memcmp(test.c_str(), reply.to_char(), test.length()) != 0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        print_case_failed("%s:%s",test.c_str(),reply.to_char());
    }
//read three    int read(void * buf, int nbyte, SocketAddress & address);
    SocketAddress socket_address_three(address_info);
    len = local_host_socket_udp_server.read(reply.data(),reply.size(),socket_address_three);
    if( len < 0 ){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return false;
    }
    ip_address = socket_address_three.address_to_string();
    print_case_message("case recv from %s:%d ", ip_address.to_char(),socket_address_two.port());
    print_case_message("read '%s' from udp socket ", reply.to_char());
    test.to_lower();//value must be changed
    if( memcmp(test.c_str(), reply.to_char(), test.length()) != 0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        print_case_failed("%s:%s",test.c_str(),reply.to_char());
    }
    Timer::wait_milliseconds(100);
    local_host_socket_udp_client.close();
    local_host_socket_udp_server.close();
    return true;
}
#define PING_PKT_S 64
typedef struct MCU_PACK{
  u8 type;
  u8 code;
  u16 chksum;
  u16 id;
  u16 seqno;
}icmp_echo_hdr_t;
typedef struct MCU_PACK{
    icmp_echo_hdr_t hdr;
    u8 data[PING_PKT_S - sizeof(icmp_echo_hdr_t)];
}ping_pckt_t;
static u16 ping_checksum(void *b, int len);
static u16 ping_checksum(void *b, int len)
{
    u16 *buf = (u16*)b;
    u32 sum=0;
    u16 result;
    for ( sum = 0; len > 1; len -= 2 ){
        sum += *buf;
        buf++;
    }
    if ( len == 1 ){
        sum += *(u8*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}
static StructuredData<ping_pckt_t> make_ping_packet();
static StructuredData<ping_pckt_t> make_ping_packet(){
    #define ICMP_ECHO 8    /* echo */
    static u16 seq_no = 0;
    StructuredData<ping_pckt_t> pckt_header;
    pckt_header.fill(0);
    pckt_header->hdr.type = ICMP_ECHO;
    pckt_header->hdr.id = Thread::get_pid();
    u32 msg_len = PING_PKT_S - sizeof (icmp_echo_hdr_t);
    for ( u8 i = 0; i < (msg_len-1); i++ ) {
        pckt_header->data[i] = (u8)((u8)seq_no + i+'0');
    }
    pckt_header->data[msg_len-1] = 0;
    pckt_header->hdr.seqno= seq_no;
    seq_no++;
    pckt_header->hdr.chksum = ping_checksum(pckt_header.to_char(), pckt_header.size());
    return pckt_header;
}

/* @brief raw socket case
 * listen port - 5004
 * conection to port - 5005
 * */
bool SocketTest::execute_socket_case_raw(){
    /*      start raw connection handing
     *      use two sockets for write and read message
    */
    Timer::wait_milliseconds(100);
    SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_RAW,\
                                   SocketAddressInfo::PROTOCOL_ICMP,AI_PASSIVE);
    SocketOption socket_option(SocketOption::LEVEL_SOCKET);
    socket_option = socket_option.set_send_size(64);
    ClockTime clock_time,clock_time_add;
    clock_time = Clock::get_time();
    clock_time_add.set(0,50*1000*1000);
    clock_time +=clock_time_add;
    socket_option = socket_option.set_receive_timeout(clock_time);
    Vector<SocketAddressInfo> list = address_info.fetch_node("stratifylabs.co");
    if(list.size()<=0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return false;
    }
    SocketAddress localhost_raw_address_client(list.at(0));
    Socket local_host_socket_raw_client;
    //socket with sending address info
    if(local_host_socket_raw_client.create(localhost_raw_address_client)!=0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return false;
    }
    StructuredData<ping_pckt_t> ping_send = make_ping_packet();
    int len_writed;
    local_host_socket_raw_client << socket_option;
    len_writed = local_host_socket_raw_client.write(ping_send.to_char(),ping_send.size(),\
                                                    localhost_raw_address_client);
    if( len_writed != (int)ping_send.size() ){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return false;
    }
    int len;
    struct sockaddr ai_addr;
    socklen_t ai_addrlen;
    if(family == SocketAddressInfo::FAMILY_NONE||
       family == SocketAddressInfo::FAMILY_INET){
        ai_addrlen = sizeof(sockaddr_in);
    }else{
        ai_addrlen = sizeof(sockaddr_in6);
    }

    Data read_data(256);
    read_data.fill(0);
    len = local_host_socket_raw_client.read(read_data.to_char(),read_data.size(),&ai_addr,&ai_addrlen);
    if( len < 0 ){
        print_case_failed("Failed %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,len);
        return false;
    }
    const u8 IP_HEADER_LEN = 20;
    u8 data_buf[PING_PKT_S - sizeof (icmp_echo_hdr_t) - IP_HEADER_LEN];
    for(u32 i=0;i<(len - sizeof (icmp_echo_hdr_t) - IP_HEADER_LEN);i++){
        data_buf[i] = read_data.at_u8(sizeof (icmp_echo_hdr_t) + IP_HEADER_LEN + i);
    }
    if( memcmp(ping_send->data, data_buf, ping_send.size()-sizeof (icmp_echo_hdr_t)) != 0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
    }
    local_host_socket_raw_client.close();
    return true;
}
/* @brief make udp socket and listen on
 * listen port - 5005
 * connecting to port - 5004
 * ip_address - local host
 * */
void * SocketTest::listen_on_localhost_udp(){
    /*      start udp connection handing
     *      use two sockets for write and read message
    */
    udp_server_listening = 0 ;
    SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_DGRAM,\
                                   SocketAddressInfo::PROTOCOL_UDP,0);
    Vector<SocketAddressInfo> list = address_info.fetch("localhost",case_listen_port_str);
    if(list.size()<=0){
        print_case_failed("list error thread");
        return 0;
    }
    Socket local_host_socket_udp_server;
    Socket local_host_socket_udp_client;
    //address connect to
    SocketAddress localhost_udp_address_client(list.at(0), case_listen_port);
    list.free();
    list.clear();
    list = address_info.fetch("localhost",thread_listen_port_str);
    if(list.size()<=0){
        print_case_failed("list error thread");
        return 0;
    }
    //address for listening
    SocketAddress localhost_udp_address_server(list.at(0), thread_listen_port);
    if(local_host_socket_udp_client.create(localhost_udp_address_client)!=0){
        print_case_failed("create error socket");
        return 0;
    }
    if(local_host_socket_udp_server.create(localhost_udp_address_server)!=0){
        print_case_failed("create error socket");
        return 0;
    }
    if( local_host_socket_udp_server.bind_and_listen(localhost_udp_address_server) < 0 ){
        print_case_failed("Failed to bind to localhost (%d)");
        return 0;
    }

    Data reply(256);
    int len_hand;
    struct sockaddr ai_addr;
    socklen_t ai_addrlen;
    if(family == SocketAddressInfo::FAMILY_NONE||
       family == SocketAddressInfo::FAMILY_INET){
        ai_addrlen = sizeof(sockaddr_in);
    }else{
        ai_addrlen = sizeof(sockaddr_in6);
    }

    len_hand = local_host_socket_udp_server.read(reply.data(),reply.size(),&ai_addr,&ai_addrlen);
    //len_hand = local_host_socket_udp_server.read(reply.data(),reply.size(),s_address);
    if( len_hand < 0 ){
        print_case_failed("Failed to read client socket %d",len_hand);
        return 0;
    }
    u16 port;
    port = ai_addr.sa_data[1]<<8 | ai_addr.sa_data[2];
    //client was port to connect
    //send one
    Timer::wait_milliseconds(100);
    len_hand = local_host_socket_udp_client.write(reply.data(),len_hand,localhost_udp_address_client);
    if(len_hand  < 0 ){
        print_case_failed("Failed to write client socket ");
        return 0;
    }
    //send two
    String send_data(reply.to_char());
    send_data.to_upper();//change sending data
    Timer::wait_milliseconds(100);
    len_hand = local_host_socket_udp_client.write(send_data.to_char(),len_hand,localhost_udp_address_client);
    if(len_hand  < 0 ){
        print_case_failed("Failed to write client socket ");
        return 0;
    }
    //send three
    Timer::wait_milliseconds(100);
    send_data.to_lower();//change data
    len_hand = local_host_socket_udp_client.write(send_data.to_char(),len_hand,localhost_udp_address_client);
    if(len_hand  < 0 ){
        print_case_failed("Failed to write client socket ");
        return 0;
    }
    local_host_socket_udp_client.close();
    local_host_socket_udp_server.close();
    return 0;
}

/*@brief stress test for sockets
 *
 * */
bool SocketTest::execute_class_stress_case(){
    SocketAddressInfo address_info(SocketAddressInfo::FAMILY_INET,SocketAddressInfo::TYPE_STREAM,\
                                   SocketAddressInfo::PROTOCOL_TCP,AI_PASSIVE);
    Vector<SocketAddressInfo> list;
    Socket local_host_socket_client;
    Thread thread;
    int i;
    int itterate_numm = 100;
    for(int i =0;i<itterate_numm;i++){
        list = address_info.fetch("localhost", case_listen_port_str);
        if(list.size()<=0){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
            list.free();
            list.clear();
            return false;
        }
        SocketAddress localhost_address(list.at(0),  case_listen_port);
        if(local_host_socket_client.create(localhost_address)!=0){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
            return false;
        }
        local_host_socket_client.close();
        list.free();
        list.clear();
    }


    for(int j=0;j<3;j++){
        switch(j){
        case(0):
            family = SocketAddressInfo::FAMILY_NONE;
            break;
        case(1):
            family = SocketAddressInfo::FAMILY_INET;
            break;
        case(2):
            family = SocketAddressInfo::FAMILY_INET6;
            break;
        }
        thread_running = true;
        if( thread.create(listen_on_localhost_thread_function_ping_pong, this) < 0 ){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            return false;
        }

        Timer::wait_milliseconds(100);

        address_info.set_family(family);
        list = address_info.fetch_node("localhost");
        if(list.size()<=0){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            list.free();
            list.clear();
            return 0;
        }
        SocketAddress localhost_address_client(list.at(0),  thread_listen_port_tcp);
        if(local_host_socket_client.create(localhost_address_client)!=0){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            return false;
        }

        if( local_host_socket_client.connect(localhost_address_client) < 0 ){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            return false;
        }
        Timer::wait_milliseconds(50);
        String test("begin");
        Data reply(MAX_PACKET_SIZE);
        for (i = 0;i<itterate_numm;i++){
            int len;
            len = 0;
            //sending all data
            while(len < test.length()){
                len += local_host_socket_client.write((test.to_char() + len),test.length()-len);
                if(len >0){

                }else{
                    print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                    break;
                }
            }
            reply.fill(0);
            len = 0;
            //receiving all data
            while(len<test.length()){
                int len_temp;
                Data reply_temp(MAX_PACKET_SIZE);
                len_temp = local_host_socket_client.read(reply_temp);
                if(len_temp>0){
                    if(len){
                        reply.append(reply_temp);
                    }else{
                        reply.copy_contents(reply_temp);
                    }
                    len += len_temp;
                }else{
                    print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                    break;
                }
            }
            if( memcmp(test.to_char(), reply.to_char(), test.length()) != 0){
                print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                break;
            }
            test.append('a');
        }
        print_case_message("send recv %d packet",i);

        for (i = 0;i<itterate_numm;i++){
            int len;
            u16 size = (u16)(rand()&0x3ff);
            size = size>1?size:5;
            rand_string_value(size,test);
            len = 0;
            //sending all data
            while(len < test.length()){
                len += local_host_socket_client.write((test.to_char() + len),test.length()-len);
                if(len >0){

                }else{
                    print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                    break;
                }
            }
            reply.fill(0);
            len = 0;
            //receiving all data
            while(len<test.length()){
                int len_temp;
                Data reply_temp(MAX_PACKET_SIZE);
                len_temp = local_host_socket_client.read(reply_temp);
                if(len_temp>0){
                    if(len){
                        reply.append(reply_temp);
                    }else{
                        reply.copy_contents(reply_temp);
                    }
                    len += len_temp;
                }else{
                    print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                    break;
                }
            }
            if( memcmp(test.to_char(), reply.to_char(), test.length()) != 0){
                print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                break;
            }
        }
        print_case_message("send recv %d packet",i);
        thread_running = false;
        Timer::wait_milliseconds(100);
        local_host_socket_client.close();
    }
    Timer::wait_milliseconds(100);
    //next part use udp socket
    for(int j=0;j<2;j++){
        switch(j){
        case(0):
            family = SocketAddressInfo::FAMILY_INET;
            break;
        case(1):
            family = SocketAddressInfo::FAMILY_INET6;
            break;
        }
        SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_DGRAM,\
                                       SocketAddressInfo::PROTOCOL_UDP,AI_PASSIVE);

        thread_running = true;
        if( thread.create(listen_on_localhost_thread_function_ping_pong_udp, this) < 0 ){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            return false;
        }
        Timer::wait_milliseconds(50);

        address_info.set_family(family);
        list.free();
        list.clear();
        list = address_info.fetch("localhost",thread_listen_port_str);
        if(list.size()<=0){
            return false;
        }
        Socket local_host_socket_udp_server;
        Socket local_host_socket_udp_client;
        //address connect to
        SocketAddress localhost_udp_address_client(list.at(0), thread_listen_port);
        list.free();
        list.clear();
        list = address_info.fetch("localhost",case_listen_port_str);
        if(list.size()<=0){
            return false;
        }
        //address for listening
        SocketAddress localhost_udp_address_server(list.at(0), case_listen_port);
        //socket with sending address info
        if(local_host_socket_udp_client.create(localhost_udp_address_client)!=0){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            return false;
        }
        //socket with reading address info
        if(local_host_socket_udp_server.create(localhost_udp_address_server)!=0){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            return false;
        }
        if( local_host_socket_udp_server.bind_and_listen(localhost_udp_address_server) < 0 ){
            print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,j);
            return 0;
        }

        Timer::wait_milliseconds(10);
        String test("begin");
        Data reply(MAX_PACKET_SIZE);
        for (i = 0;i<itterate_numm;i++){
            int len;
            u16 size = (u16)(rand()&0xff);
            size = size>1?size:5;
            rand_string_value(size,test);
            len = 0;
            //sending all data
            while(len < test.length()){
                len += local_host_socket_udp_client.write((test.to_char() + len),test.length()-len,localhost_udp_address_client);
                if(len >0){

                }else{
                    print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                    break;
                }
            }
            reply.fill(0);
            len = 0;
            //receiving all data
            while(len<test.length()){
                int len_temp;
                Data reply_temp(MAX_PACKET_SIZE);
                len_temp = local_host_socket_udp_server.read(reply_temp);
                if(len_temp>0){
                    if(len){
                        reply.append(reply_temp);
                    }else{
                        reply.copy_contents(reply_temp);
                    }
                    len += len_temp;
                }else{
                    print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                    break;
                }
            }
            if( memcmp(test.to_char(), reply.to_char(), test.length()) != 0){
                print_case_failed("Failed cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
                break;
            }
        }
        print_case_message("send recv %d packet",i);
        thread_running = false;
        local_host_socket_udp_client.close();
        local_host_socket_udp_server.close();
        Timer::wait_milliseconds(100);
    }
    family = SocketAddressInfo::FAMILY_INET;
    for(int i=0;i<itterate_numm;i++){
        if(!execute_socket_case_raw()){
            print_case_failed("raw socket case failed");
            print_case_failed("Failed in cycle %s:%d:%d", __PRETTY_FUNCTION__, __LINE__,i);
            break;
        }
    }

    return true;
}
/* @brief recv -> reply
 * use global option for inet family type
 * using for stress test
 * */
void * SocketTest::listen_on_localhost_ping_pong(){
    SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_STREAM,\
                                   SocketAddressInfo::PROTOCOL_TCP,AI_PASSIVE);

    Vector<SocketAddressInfo> list = address_info.fetch_node("localhost");

    if( list.count() == 0 ){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }

    Socket local_host_listen_socket;
    SocketAddress localhost_address(list.at(0), thread_listen_port_tcp);
    if( local_host_listen_socket.create(localhost_address) < 0 ){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }
    print_case_message("create socket at %s", localhost_address.address_to_string().str());
    if( local_host_listen_socket.bind_and_listen(localhost_address) < 0 ){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }
    print_case_message("Listening on localhost:%d", localhost_address.port());
    //now accept -- this will block until a request arrives
    SocketAddress accepted_address;
    Socket local_host_session_socket = local_host_listen_socket.accept(accepted_address);

    print_case_message("Accepted connection from %s:%d", accepted_address.address_to_string().str(), accepted_address.port());
    Data incoming(MAX_PACKET_SIZE);
    while(thread_running){
        int len_recv;
        len_recv = local_host_session_socket.read(incoming);
        if(len_recv>0){
            local_host_session_socket.write(incoming);
        }else if (len_recv == 0){
            //disconnect from client
            break;
        }else{
            print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
            break;
        }
        incoming.fill(0);
    }

    if(local_host_session_socket.close()<0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }

    if( local_host_listen_socket.close() < 0 ){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }
    return 0;
}
/* @brief recv -> reply
 * use global option for inet family type
 * using for stress test
 * listen port - 5005
 * connecting to port - 5004
 * ip_address - local host
 * */
void * SocketTest::listen_on_localhost_ping_pong_udp(){
/*      start udp connection handing
 *      use two sockets for write and read message
*/
    udp_server_listening = 0 ;
    SocketAddressInfo address_info(family,SocketAddressInfo::TYPE_DGRAM,\
                                   SocketAddressInfo::PROTOCOL_UDP,0);
    Vector<SocketAddressInfo> list = address_info.fetch("localhost",case_listen_port_str);
    if(list.size()<=0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }
    Socket local_host_socket_udp_server;
    Socket local_host_socket_udp_client;
    //address connect to
    SocketAddress localhost_udp_address_client(list.at(0), case_listen_port);
    list.free();
    list.clear();
    list = address_info.fetch("localhost",thread_listen_port_str);
    if(list.size()<=0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }
    //address for listening
    SocketAddress localhost_udp_address_server(list.at(0), thread_listen_port);
    if(local_host_socket_udp_client.create(localhost_udp_address_client)!=0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }
    if(local_host_socket_udp_server.create(localhost_udp_address_server)!=0){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }
    if( local_host_socket_udp_server.bind_and_listen(localhost_udp_address_server) < 0 ){
        print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
        return 0;
    }

    Data reply(256);
    int len_hand;
    struct sockaddr ai_addr;
    socklen_t ai_addrlen;
    if(family == SocketAddressInfo::FAMILY_NONE||
       family == SocketAddressInfo::FAMILY_INET){
        ai_addrlen = sizeof(sockaddr_in);
    }else{
        ai_addrlen = sizeof(sockaddr_in6);
    }
    while(thread_running){

        len_hand = local_host_socket_udp_server.read(reply.data(),reply.size(),&ai_addr,&ai_addrlen);
        if( len_hand < 0 ){
            print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
            return 0;
        }
        //client was port to connect
        Timer::wait_milliseconds(1);
        len_hand = local_host_socket_udp_client.write(reply.data(),len_hand,localhost_udp_address_client);
        if(len_hand  < 0 ){
            print_case_failed("Failed %s:%d", __PRETTY_FUNCTION__, __LINE__);
            return 0;
        }
    }
    local_host_socket_udp_client.close();
    local_host_socket_udp_server.close();
    return 0;
}

static void rand_string_value(u16 size,String & string){
    string.clear();
    for (u16 i =0;i<size;i++){
        u8 value;
        value = (u8)(rand()%25);
        value +=97;
        string.append(value);
    }
}

