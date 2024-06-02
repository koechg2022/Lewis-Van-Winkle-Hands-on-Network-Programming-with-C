
#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
    #define crap_os

    #if !defined(_WIN32_WINNT)
        #define _WIN32_WINNT 0x0600
    #endif

    // #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>

    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")

    #include <windows.h>
    #include <locale>
    #include <codecvt> 

    std::string convert_wide_to_utf8(const wchar_t* wide_str) {
        int utf8_length = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8_str(utf8_length, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, utf8_str.data(), utf8_length, nullptr, nullptr);
        return utf8_str;
    }

    // micros and macros for portability
    #define adapter_type PIP_ADAPTER_ADDRESSES
    #define address_type PIP_ADAPTER_UNICAST_ADDRESS

    
    #define get_address(this_adapter) (this_adapter->FirstUnicastAddress)
    #define get_next_adapter(this_adapter) (this_adapter->Next)
    #define get_adapter_name(this_adapter) ({ \
        const wchar_t* pwchar_temp = this_adapter->FriendlyName; \
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter; \
        std::string result = converter.to_bytes(pwchar_temp); \
        result; \
    })
    #define get_ip_version(this_address) (this_address->Address.lpSockaddr->sa_family)
    #define get_next_address(this_address) (this_address->Next)
    #define get_name_info(this_address, the_buffer, the_buffer_size) (getnameinfo(this_address->Address.lpSockaddr, this_address->Address.iSockaddrLength, the_buffer, the_buffer_size, 0, 0, NI_NUMERICHOST))
    #define free_adapters(the_adapters) (free(the_adapters))


#else
    
    #define unix_os

    #if !defined(_IFADDRS_H)
        #include <ifaddrs.h>
    #endif

    #if !defined(_SYS_SOCKET_H_)
        #include <sys/socket.h>
    #endif

    #if !defined(_NETDB_H_)
        #include <netdb.h>
    #endif

    // micros and macros for portability
    #define adapter_type struct ifaddrs*
    #define address_type struct ifaddrs*

    #define get_address(this_adapter) (this_adapter)
    #define get_next_adapter(this_adapter) (this_adapter->ifa_next)
    #define get_adapter_name(this_adapter) (std::string(this_adapter->ifa_name))
    #define get_ip_version(this_address) (this_address->ifa_addr->sa_family)
    #define get_next_address(this_address) (this_address->ifa_next)
    #define get_name_info(this_address, the_buffer, the_buffer_size) (getnameinfo(this_address->ifa_addr, (get_ip_version(this_address) == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), the_buffer, the_buffer_size, 0, 0, NI_NUMERICHOST))
    #define free_adapters(the_adapters) (freeifaddrs(the_adapters))




#endif

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <string>

#define buffer_size 100


std::map<std::string, std::map<std::string, std::set<std::string> > > get_machine_adapters();


void list_machine_adapters(std::map<std::string, std::map<std::string, std::set<std::string> > > machine_adapters = get_machine_adapters());


int main(void) {
    #if defined(crap_os)
        WSADATA d;
        if (WSAStartup(MAKEWORD(2, 2), &d)) {
            fprintf(stderr, "Yo... windows SUCKS!! It couldn't even initialize the network environment.\n");
            exit(EXIT_FAILURE);
        }
    #endif
    list_machine_adapters();
    #if defined(crap_os)
        WSACleanup();
    #endif

    return 0;
}


void list_machine_adapters(std::map<std::string, std::map<std::string, std::set<std::string> > > machine_adapters) {
    // UNDER CONSTRUCTION
    for (std::map<std::string, std::map<std::string, std::set<std::string> > >:: iterator adapt_names = machine_adapters.begin(); adapt_names != machine_adapters.end(); adapt_names++) {
        printf("%s:\n", adapt_names->first.c_str());
        for (std::map<std::string, std::set<std::string> >:: iterator ip_ver = adapt_names->second.begin(); ip_ver != adapt_names->second.end(); ip_ver++) {
            printf("\t%s:\n", ip_ver->first.c_str());
            for (std::set<std::string>::iterator ip_addr = ip_ver->second.begin(); ip_addr != ip_ver->second.end(); ip_addr++) {
                printf("\t\t%s\n", ip_addr->c_str());
            }
        }
    }
}

std::map<std::string, std::map<std::string, std::set<std::string> > > get_machine_adapters() {
    
    
    adapter_type this_machine;
    adapter_type this_adapter;

    #if defined(crap_os)
        DWORD size = 20000;
        this_machine = NULL;
        while (!this_machine) {
            this_machine = (adapter_type) malloc(size);

            if (!this_machine) {
                fprintf(stderr, "Could not allocate space for a list of the adapters on this crappy machine.\n");
                exit(EXIT_FAILURE);
            }

            int got_adapts = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0, this_machine, &size);

            if (got_adapts == ERROR_BUFFER_OVERFLOW) {
                fprintf(stderr, "Need more memory for this machine's adapters. Will try to get more memory...\n");
                free(this_machine);
            }

            else if (got_adapts == ERROR_SUCCESS) {
                break;
            }

            else {
                fprintf(stderr, "Unsurprisingly, an unexpected error occured. Your windows machine experienced Error %d\n", got_adapts);
                free(this_machine);
                exit(EXIT_FAILURE);
            }
        }
    #else
        if (getifaddrs(&this_machine)) {
            fprintf(stderr, "Could not retrieve this machine's adapter information. Error %d\n", errno);
            exit(EXIT_FAILURE);
        }
    #endif

    // this_machine is filled with adapters.
    const std::string ip_4 = "IP Version 4";
    const std::string ip_6 = "IP Version 6";
    std::map<std::string, std::map<std::string, std::set<std::string> > > the_answer;
    std::string adapter_name, ip_address, ip_ver;
    address_type this_address;
    char ip_addr_buffer[buffer_size];

    for (this_adapter = this_machine; this_adapter; this_adapter = get_next_adapter(this_adapter)) {
        if ((get_ip_version(get_address(this_adapter)) != AF_INET) && (get_ip_version(get_address(this_adapter)) != AF_INET6)) {
            continue;
        }
        for (this_address = get_address(this_adapter); this_address; this_address = get_next_address(this_address)) {
            adapter_name = get_adapter_name(this_adapter);
            ip_ver = (get_ip_version(this_address) == AF_INET) ? ip_4 : ip_6;
            memset(ip_addr_buffer, 0, buffer_size);
            get_name_info(this_address, ip_addr_buffer, buffer_size);
            ip_address = std::string(ip_addr_buffer);

            if (the_answer.find(adapter_name) == the_answer.end()) {
                std::map<std::string, std::set<std::string> > internal_map;
                std::set<std::string> internal_set;
                internal_set.insert(ip_address);
                internal_map.insert(std::make_pair(ip_ver, internal_set));
                the_answer.insert(std::make_pair(adapter_name, internal_map));
                continue;
            }

            if (the_answer[adapter_name].find(ip_ver) == the_answer[adapter_name].end()) {
                std::set<std::string> internal_set;
                internal_set.insert(ip_address);
                the_answer[adapter_name].insert(std::make_pair(ip_ver, internal_set));
                continue;
            }

            the_answer[adapter_name][ip_ver].insert(ip_address);

        }
    }

    free_adapters(this_machine);

    return the_answer;
}