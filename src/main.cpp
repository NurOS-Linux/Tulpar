/* NurOS/Tulpar/main.cpp ruzen42 */
#include <iostream>
#include "argparse.hpp"
#include "utils.hpp"
#include "colors.hpp"

int main(int argc, const char* argv[])
{
  argparse::ArgumentParser program("tulpar", "0.1.0", argparse::default_arguments::all);
  // Must be the name of binary 

  argparse::ArgumentParser install_command("install");

  install_command.add_argument("pkg")
      .help("Package name to install")
      .default_value(std::string("")); 

  argparse::ArgumentParser remove_command("remove");
  remove_command.add_argument("pkg")
      .help("Package name to remove")
      .default_value(std::string("")); 

  argparse::ArgumentParser update_command("update");
  update_command.add_argument("pkg")
      .help("Package name to update")
      .default_value(std::string("@world")); 

  argparse::ArgumentParser search_command("search");
  search_command.add_argument("pkg")
      .help("Package name to searching in data base")
      .default_value(std::string("")); 
  
  program.add_subparser(install_command);
  program.add_subparser(remove_command);
  program.add_subparser(update_command);
  program.add_subparser(search_command);

  try
  {
    program.parse_args(argc, argv);
  }
  catch (const std::exception& err) 
  {
    std::cout << COLOR_RED << "Error: " << COLOR_RESET << err.what() <<  "\n";
  }

  if (program.is_subcommand_used("install"))
  {
    auto pkg = install_command.get<std::string>("pkg");
    utils::install_package(pkg);
    std::cout << nodeps << "\n";
  } 
  else if (program.is_subcommand_used("remove")) 
  {
    auto pkg = remove_command.get<std::string>("pkg");
    utils::install_package(pkg);
  }
  else if (program.is_subcommand_used("update")) 
  {
    auto pkg = update_command.get<std::string>("pkg");
    utils::install_package(pkg);
  }
  else if (program.is_subcommand_used("search")) 
  {
    auto pkg = search_command.get<std::string>("pkg");
    utils::search_package(pkg);
  }
  else
  {
        std::cerr << COLOR_RED << "Error: " << COLOR_RESET << "No valid command provided\n";
        std::cerr << program;
        return 1;
  }

  return 0;
}
