#include <stdio.h>

#include <sapi/sys.hpp>
#include <sapi/test.hpp>

enum test_flags {
   THIS_TEST = (1<<5),
   THAT_TEST = (1<<6)
};

u32 decode_cli(const Cli & cli);

int main(int argc, char * argv[]){
    Cli cli(argc, argv);
    cli.set_publisher("Stratify Labs, Inc");
    cli.handle_version();

	 u32 o_flags;
	 o_flags = decode_cli(cli);
	 (void)o_flags;

    Test::initialize(
             Test::Name(cli.name()),
             Test::Version(cli.version()),
             Test::GitHash(SOS_GIT_HASH)
             );

    Test::finalize();
    return 0;

}

u32 decode_cli(const Cli & cli){
   u32 o_flags = 0;
   o_flags = Test::parse_execution_flags(cli);
   o_flags |= Test::parse_test(cli, "line", 0);
   o_flags |= Test::parse_test(cli, "vector", 0);
   return o_flags;
}




