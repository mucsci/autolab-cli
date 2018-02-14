#include <cmath>
#include <ctime>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "autolab/autolab.h"
#include "autolab/client.h"
#include "logger.h"

#include "app_credentials.h"
#include "build_config.h"
#include "cmdargs.h"
#include "context_manager.h"
#include "file_utils.h"
#include "pretty_print.h"

/* globals */
Autolab::Client client(server_domain, client_id, client_secret, redirect_uri, store_tokens);

bool init_autolab_client() {
  std::string at, rt;
  if (!load_tokens(at, rt)) return false;
  client.set_tokens(at, rt);
  return true;
}

/* help texts */
void print_help() {
  Logger::info << "usage: autolab [OPTIONS] <command> [command-args] [command-opts]" << Logger::endl
    << Logger::endl
    << "commands:" << Logger::endl
    << "  courses             List all courses" << Logger::endl
    << "  assessments/asmts   List all assessments of a course" << Logger::endl
    << "  status              Show status of the local assessment" << Logger::endl
    << "  problems            List all problems in an assessment" << Logger::endl
    << "  download            Download files needed for an assessment" << Logger::endl
    << "  submit              Submit a file to an assessment" << Logger::endl
    << "  scores              Show scores got on an assessment" << Logger::endl
    << "  setup               Setup the user of the client" << Logger::endl
    << Logger::endl
    << "options:" << Logger::endl
    << "  -h,--help      Show this help message" << Logger::endl
    << "  -v,--version   Show the version number of this build" << Logger::endl
    << Logger::endl
    << "run 'autolab <command> -h' to view usage instructions for each command." << Logger::endl;
}

void print_version() {
  Logger::info << "autolab-cli version " << VERSION_MAJOR << "." << VERSION_MINOR << Logger::endl;
}

void print_not_in_asmt_dir_error() {
  Logger::fatal << "Not inside an autolab assessment directory: .autolab-asmt not found" << Logger::endl
    << Logger::endl
    << "Please change directory or specify the course and assessment names" << Logger::endl;
}

/* helpers */
int perform_device_flow(Autolab::Client &client) {
  Logger::info << "Initiating authorization..." << Logger::endl << Logger::endl;
  std::string user_code, verification_uri;
  client.device_flow_init(user_code, verification_uri);
  Logger::info << "Please visit "
    << Logger::CYAN << verification_uri << Logger::NONE << " and enter the code: "
    << Logger::CYAN << user_code << Logger::NONE << Logger::endl;
  Logger::info << Logger::endl << "Waiting for user authorization ..." << Logger::endl;

  int res = client.device_flow_authorize(300); // wait for 5 minutes max
  switch (res) {
    case 1:
      Logger::info << Logger::RED << "User denied authorization." << Logger::NONE << Logger::endl;
      return 1;
    case -2:
      Logger::info << Logger::RED << "Timed out while waiting for user action. Please try again."
        << Logger::NONE << Logger::endl;
      return 1;
  }

  // res == 0
  Logger::info << Logger::GREEN << "Received authorization!" << Logger::NONE << Logger::endl;

  return 0;
}

/* exit if failed to parse */
void parse_course_and_asmt(std::string raw_input, std::string &course, std::string &asmt) {
  std::string::size_type split_pos = raw_input.find(":");
  if (split_pos == std::string::npos) {
    Logger::fatal << "Failed to parse course name and assessment name: " << raw_input << Logger::endl;
    exit(0);
  }
  course = raw_input.substr(0, split_pos);
  asmt = raw_input.substr(split_pos + 1, std::string::npos);
}

/* if the supplied names are empty, it assigns them the values from the autolab asmt file.
 * if the context file doesn't exist, it reports an error and exits.
 * If the user does specify names and they don't match, it reports an error and exits.
 */
void check_names_with_asmt_file(std::string &course_name, std::string &asmt_name) {
  bool user_specified_names = (course_name.length() > 0) || (asmt_name.length() > 0);
  std::string course_name_config, asmt_name_config;
  // attempt to load names from asmt-file
  bool found_asmt_file = read_asmt_file(course_name_config, asmt_name_config);
  if (!found_asmt_file && !user_specified_names) {
    print_not_in_asmt_dir_error();
    exit(0);
  }

  if (found_asmt_file && user_specified_names) {
    if ((course_name != course_name_config) || (asmt_name != asmt_name_config)) {
      Logger::fatal << "The provided names and the configured names for this autolab assessment directory do not match:" << Logger::endl
        << "Provided names:   " << course_name  << ":" << asmt_name << Logger::endl
        << "Configured names: " << course_name_config << ":" << asmt_name_config << Logger::endl << Logger::endl
        << "Please resolve this conflict, or use the '-f' option to force the use of the provided names." << Logger::endl;
      exit(0);
    }
  }

  if (found_asmt_file && !user_specified_names) {
    course_name = course_name_config;
    asmt_name = asmt_name_config;
  }

  // if !found_asmt_file && user_specified_names, we don't need to do anything
}

/* commands */

int show_status(cmdargs &cmd) {
  cmd.setup_help("autolab status",
      "Show the context of the current directory. If inside an assessment "
      "directory, the details of the assessment will be shown.");
  cmd.setup_done();

  // set up logger
  Logger::fatal.set_prefix("Cannot show status");

  std::string course_name, asmt_name;
  bool in_asmt_dir = read_asmt_file(course_name, asmt_name);
  if (!in_asmt_dir) {
    Logger::info << "Not currently in any assessment directory" << Logger::endl
      << Logger::endl
      << "Failed to find an assessment config file in the current directory or any" << Logger::endl
      << "of its parent directories (up to " << DEFAULT_RECUR_LEVEL << " levels)." << Logger::endl;
    return 0;
  }

  Logger::info << "Assessment Config: " << course_name << ":" << asmt_name
    << Logger::endl << Logger::endl;

  // get details
  Autolab::DetailedAssessment dasmt;
  client.get_assessment_details(dasmt, course_name, asmt_name);

  Logger::info << dasmt.asmt.display_name << Logger::endl
    << "Due: " << std::ctime(&dasmt.asmt.due_at) // ctime ends string with '\n'
    << "Max submissions: ";

  if (dasmt.max_submissions < 0) {
    Logger::info << "Infinite" << Logger::endl;
  } else {
    Logger::info << dasmt.max_submissions << Logger::endl;
  }
     
  Logger::info << "Max grace days: " << dasmt.max_grace_days << Logger::endl;

  return 0;
}

/* download assessment into a new directory
 */
int download_asmt(cmdargs &cmd) {
  cmd.setup_help("autolab download",
      "Create a directory for working on the specified assessment. The writeup "
      "and the handout are downloaded into the directory if they are files. "
      "The assessment directory is also setup with a local config so that "
      "running 'autolab submit <filename>' works without the need to specify "
      "the names of the course and assessment.");
  cmd.new_arg("course_name:assessment_name", true);
  cmd.setup_done();

  // set up logger
  Logger::fatal.set_prefix("Cannot download assessment");

  // parse course and assessment name
  std::string course_name, asmt_name;
  parse_course_and_asmt(cmd.args[2], course_name, asmt_name);

  Logger::info << "Querying assessment '" << asmt_name << "' of course '" << 
    course_name << "' ..." << Logger::endl;

  // make sure assessment exists
  Autolab::DetailedAssessment dasmt;
  client.get_assessment_details(dasmt, course_name, asmt_name);

  // setup directory
  bool dir_exists = dir_find(get_curr_dir(), asmt_name.c_str(), true);
  if (dir_exists) {
    Logger::fatal << "Directory named '" << asmt_name << "' already exists. "
      << "Please delete or rename before proceeding." << Logger::endl;
    return 0;
  }

  std::string new_dir(get_curr_dir());
  new_dir.append("/" + asmt_name);
  Logger::info << "Creating directory " << new_dir << Logger::endl;

  create_dir(new_dir.c_str());

  // download files into directory
  Autolab::Attachment handout, writeup;
  client.download_handout(handout, new_dir, course_name, asmt_name);
  switch (handout.format) {
    case Autolab::AttachmentFormat::none:
      Logger::info << "Assessment has no handout" << Logger::endl;
      break;
    case Autolab::AttachmentFormat::url:
      Logger::info << "Handout URL: " << handout.url << Logger::endl;
      break;
    case Autolab::AttachmentFormat::file:
      Logger::info << "Handout downloaded into assessment directory" << Logger::endl;
      break;
  }

  client.download_writeup(writeup, new_dir, course_name, asmt_name);
  switch (writeup.format) {
    case Autolab::AttachmentFormat::none:
      Logger::info << "Assessment has no writeup" << Logger::endl;
      break;
    case Autolab::AttachmentFormat::url:
      Logger::info << "Writeup URL: " << writeup.url << Logger::endl;
      break;
    case Autolab::AttachmentFormat::file:
      Logger::info << "Writeup downloaded into assessment directory" << Logger::endl;
      break;
  }

  // write assessment file
  write_asmt_file(new_dir, course_name, asmt_name);

  // additional info
  Logger::info << Logger::endl << "Due: " << std::ctime(&dasmt.asmt.due_at);

  return 0;
}

/* two ways of calling:
 *   1. autolab submit <filename>                  (must have autolab-asmt file)
 *   2. autolab submit <course>:<asmt> <filename>  (from anywhere)
 */
int submit_asmt(cmdargs &cmd) {
  cmd.setup_help("autolab submit",
      "Submit a file to an assessment. The course and assessment names are not "
      "needed if the current directory or its ancestor directories includes an "
      "assessment config file. The operation fails if the specified names and "
      "the config file do not match, unless the '-f' option is used, in which "
      "case the assessment config file is ignored.");
  cmd.new_arg("course_name:assessment_name", false);
  cmd.new_arg("filename", true);
  bool option_force = cmd.new_flag_option("-f","--force", "Force use the "
    "specified course:assessment pair, overriding the local config");
  cmd.setup_done();

  std::string course_name, asmt_name, filename;

  // set up logger
  Logger::fatal.set_prefix("Cannot submit assessment");

  if (cmd.nargs() >= 4) {
    // user provided course and assessment name with filename
    parse_course_and_asmt(cmd.args[2], course_name, asmt_name);
    filename = cmd.args[3];
  } else {
    // user only provided filename
    if (option_force) {
      Logger::fatal << "The '-f' option can only be used when the course and assessment names are also specified." << Logger::endl;
      return 0;
    }
    filename = cmd.args[2];
  }

  if (!option_force) {
    // only check local version if non-force
    check_names_with_asmt_file(course_name, asmt_name);
  }

  if (!file_exists(filename.c_str())) {
    Logger::fatal << "File not found: " << filename << Logger::endl;
    return 0;
  }

  Logger::info << "Submitting to " << course_name << ":" << asmt_name << " ...";
  if (option_force) {
    Logger::info << " (force)" << Logger::endl;
  } else {
    Logger::info << Logger::endl;
  }

  // conflicts resolved, use course_name and asmt_name from now on
  int version = client.submit_assessment(course_name, asmt_name, filename);

  Logger::info << Logger::GREEN << "Successfully submitted to Autolab (version " << version << ")" << Logger::NONE << Logger::endl;
  
  return 0;
}

int show_courses(cmdargs &cmd) {
  cmd.setup_help("autolab courses",
      "List all current courses of the user.");
  cmd.setup_done();

  // set up logger
  Logger::fatal.set_prefix("Cannot get courses");

  std::vector<Autolab::Course> courses;
  client.get_courses(courses);
  LogDebug("Found " << courses.size() << " current courses." << Logger::endl);

  std::string course_name_config, asmt_name_config;
  read_asmt_file(course_name_config, asmt_name_config);
  std::string course_name_config_lower = to_lowercase(course_name_config);

  for (auto &c : courses) {
    bool is_curr_asmt = (course_name_config_lower == to_lowercase(c.name));
    if (is_curr_asmt) {
      Logger::info << "* " << Logger::GREEN;
    } else {
      Logger::info << "  ";
    }

    Logger::info << c.name << " (" << c.display_name << ")" << Logger::endl;

    if (is_curr_asmt) {
      Logger::info << Logger::NONE;
    }
  }

  return 0;
}

int show_assessments(cmdargs &cmd) {
  cmd.setup_help("autolab assessments",
      "List all available assessments of a course.");
  cmd.new_arg("course_name", true);
  cmd.setup_done();

  // set up logger
  Logger::fatal.set_prefix("Cannot get assessments");

  std::string course_name(cmd.args[2]);

  std::vector<Autolab::Assessment> asmts;
  client.get_assessments(asmts, course_name);
  LogDebug("Found " << asmts.size() << " assessments." << Logger::endl);

  std::string course_name_config, asmt_name_config;
  read_asmt_file(course_name_config, asmt_name_config);
  bool is_curr_course = case_insensitive_str_equal(course_name, course_name_config);
  std::string asmt_name_config_lower = to_lowercase(asmt_name_config);

  std::sort(asmts.begin(), asmts.end(), Autolab::Utility::compare_assessments_by_name);
  for (auto &a : asmts) {
    bool is_curr_asmt = is_curr_course && (asmt_name_config_lower == to_lowercase(a.name));
    if (is_curr_asmt) {
      Logger::info << "* " << Logger::GREEN;
    } else {
      Logger::info << "  ";
    }

    Logger::info << a.name << " (" << a.display_name << ")" << Logger::endl;

    if (is_curr_asmt) {
      Logger::info << Logger::NONE;
    }
  }

  return 0;
}

int show_problems(cmdargs &cmd) {
  cmd.setup_help("autolab problems",
      "List all problems of an assessment. Course and assessment names are "
      "optional if inside an autolab assessment directory.");
  cmd.new_arg("course_name:assessment_name", false);
  cmd.setup_done();

  // set up logger
  Logger::fatal.set_prefix("Cannot get problems");
  
  std::string course_name, asmt_name;
  // user-specified names take precedence
  if (cmd.nargs() >= 3) {
    parse_course_and_asmt(cmd.args[2], course_name, asmt_name);
  } else {
    if (!read_asmt_file(course_name, asmt_name)) {
      print_not_in_asmt_dir_error();
      exit(0);
    }
  }

  std::vector<Autolab::Problem> problems;
  client.get_problems(problems, course_name, asmt_name);

  LogDebug("Found " << problems.size() << " problems." << Logger::endl);

  for (auto &p : problems) {
    Logger::info << p.name;
    if (!std::isnan(p.max_score)) {
      Logger::info << " (" << p.max_score << ")" << Logger::endl;
    } else {
      Logger::info << Logger::endl;
    }
  }

  return 0;
}

int show_scores(cmdargs &cmd) {
  cmd.setup_help("autolab scores",
      "Show all scores the user got for an assessment. Course and assessment "
      "names are optional if inside an autolab assessment directory.");
  cmd.new_arg("course_name:assessment_name", false);
  bool option_all = cmd.new_flag_option("-a", "--all",
      "Show scores from all submission. Default shows only the latest");
  cmd.setup_done();

  // set up logger
  Logger::fatal.set_prefix("Cannot get scores");

  std::string course_name, asmt_name;
  // user-specified names take precedence
  if (cmd.nargs() >= 3) {
    parse_course_and_asmt(cmd.args[2], course_name, asmt_name);
  } else {
    if (!read_asmt_file(course_name, asmt_name)) {
      print_not_in_asmt_dir_error();
      exit(0);
    }
  }

  std::vector<Autolab::Problem> problems;
  client.get_problems(problems, course_name, asmt_name);

  Logger::info << "Scores for " << course_name << ":" << asmt_name << Logger::endl
    << "(Only submissions made via this client can be shown)" << Logger::endl
    << Logger::endl;

  std::vector<std::vector<std::string>> sub_table;
  // prepare table header
  std::vector<std::string> header;
  header.push_back("version");
  for (auto &p : problems) {
    std::string column(p.name);
    if (!std::isnan(p.max_score)) {
      column += " (" + double_to_string(p.max_score, 1) + ")";
    }
    header.push_back(column);
  }
  sub_table.push_back(header);

  // get submissions
  std::vector<Autolab::Submission> subs;
  client.get_submissions(subs, course_name, asmt_name);

  LogDebug("Found " << subs.size() << " submissions." << Logger::endl);

  // prepare table body
  if (subs.size() == 0) {
    Logger::info << format_table(sub_table)
      << "[none]" << Logger::endl;
  } else {
    int nprint = option_all ? subs.size() : 1;
    for (int i = 0; i < nprint; i++) {
      std::vector<std::string> row;
      Autolab::Submission &s = subs[i];
      row.push_back(std::to_string(s.version));

      auto &scores_map = s.scores;
      for (auto &p : problems) {
        auto score = scores_map.find(p.name); // find by problem name
        if (score != scores_map.end() && !std::isnan(score->second)) {
          row.push_back(double_to_string(score->second, 1));
        } else {
          row.push_back("--");
        }
      }
      
      sub_table.push_back(row);
    }

    Logger::info << format_table(sub_table);
  }

  return 0;
}

int show_feedback(cmdargs &cmd) {
  cmd.setup_help("autolab feedback",
      "Gets feedback for a problem of an assessment. If version number is not "
      "given, the latest version will be used. If problem_name is not given, "
      "the first problem will be used. Course and assessment names are "
      "optional if inside an autolab assessment directory.");
  cmd.new_arg("course_name:assessment_name", false);
  std::string option_problem = cmd.new_option("-p", "--problem",
      "Get feedback for this problem");
  std::string option_version = cmd.new_option("-v", "--version",
      "Get feedback for this particular version");
  cmd.setup_done();

  // set up logger
  Logger::fatal.set_prefix("Cannot get feedback");

  std::string course_name, asmt_name;
  // user-specified names take precedence
  if (cmd.nargs() >= 3) {
    parse_course_and_asmt(cmd.args[2], course_name, asmt_name);
  } else {
    if (!read_asmt_file(course_name, asmt_name)) {
      print_not_in_asmt_dir_error();
      exit(0);
    }
  }

  // determine version number
  int version = -1;
  if (option_version.length() == 0) {
    // use latest version
    std::vector<Autolab::Submission> subs;
    client.get_submissions(subs, course_name, asmt_name);

    if (subs.size() == 0) {
      Logger::fatal << "No submissions available for this assessment." << Logger::endl;
      return 0;
    }

    version = subs[0].version;
  } else {
    version = std::stoi(option_version);
  }

  // determine problem name
  if (option_problem.length() == 0) {
    // use first problem
    std::vector<Autolab::Problem> problems;
    client.get_problems(problems, course_name, asmt_name);

    if (problems.size() == 0) {
      Logger::fatal << "This assessment has no problems." << Logger::endl;
      return 0;
    }

    option_problem = problems[0].name;
  }
  LogDebug("Using problem name: " << option_problem << Logger::endl);

  std::string feedback;
  client.get_feedback(feedback, course_name, asmt_name, version, option_problem);

  Logger::info << feedback << Logger::endl;
  return 0;  
}

/* must manually init client */
int user_setup(cmdargs &cmd) {
  cmd.setup_help("autolab setup",
      "Initiate user setup for the current user.");
  bool option_force = cmd.new_flag_option("-f", "--force",
      "Force user setup, removing the current user");
  cmd.setup_done();

  bool user_exists = init_autolab_client();

  if (user_exists && !option_force) {
    // perform a check if not a forced setup
    bool token_valid = true;
    Autolab::User user_info;
    try {
      client.get_user_info(user_info);
    } catch (Autolab::InvalidTokenException &e) {
      token_valid = false;
    }
    if (token_valid) {
      Logger::info << "User '" << user_info.first_name
        << "' is currently set up on this client." << Logger::endl
        << "To force reset of user info, use the '-f' option." << Logger::endl;
      return 0;
    }
  }

  // user non-existant, or existing user's credentials no longer work, or forced
  int result = perform_device_flow(client);
  if (result == 0) {
    Logger::info << Logger::endl << "User setup complete." << Logger::endl;
    return 0;
  }
  Logger::info << Logger::endl << "User setup failed." << Logger::endl;
  return -1;
}

int main(int argc, char *argv[]) {
  cmdargs cmd;
  if (!parse_cmdargs(cmd, argc, argv)) {
    Logger::fatal << "Invalid command line arguments. All options must come after" << Logger::endl
      << "all positional arguments (e.g. commands). For detailed usage, run with '-h'." << Logger::endl;
    return 0;
  }

  if (cmd.nargs() == 1) {
    // not a command
    if (cmd.has_option("-v", "--version")) {
      print_version();
      return 0;
    }
    print_help();
    return 0;
  }
  
  // determine what command it is
  std::string command(argv[1]);

  try {

    if ("setup" == command) {
      return user_setup(cmd);
    } else {
      Logger::fatal.set_prefix("Cannot start autolab client");

      if (!init_autolab_client()) {
        Logger::fatal << "No user set up on this client yet." << Logger::endl
          << Logger::endl
          << "Please run 'autolab setup' to setup your Autolab account." << Logger::endl;
        return 0;
      }
      try {
        if ("status" == command) {
          return show_status(cmd);
        } else if ("download" == command) {
          return download_asmt(cmd);
        } else if ("submit" == command) {
          return submit_asmt(cmd);
        } else if ("courses" == command) {
          return show_courses(cmd);
        } else if ("assessments" == command ||
                   "asmts" == command) {
          return show_assessments(cmd);
        } else if ("problems" == command) {
          return show_problems(cmd);
        } else if ("scores" == command) {
          return show_scores(cmd);
        } else if ("feedback" == command) {
          return show_feedback(cmd);
        } else {
          Logger::fatal << "Unrecognized command: " << command << Logger::endl;
        }
      } catch (Autolab::InvalidTokenException &e) {
        Logger::fatal << "Authorization invalid or expired." << Logger::endl
          << Logger::endl
          << "Please re-authorize this client by running 'autolab-setup'" << Logger::endl;
        return 0;
      }
    }
    
  } catch (Autolab::HttpException &e) {
    Logger::fatal << e.what() << Logger::endl;
    return -1;
  } catch (Autolab::InvalidResponseException &e) {
    Logger::fatal << Logger::endl
      << "Received invalid response from API server: " << Logger::endl
      << e.what() << Logger::endl;
    return 0;
  } catch (Autolab::ErrorResponseException &e) {
    Logger::fatal << e.what() << Logger::endl;
    return 0;
  }

  return 0;
}