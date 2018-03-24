#include "cmdmap.h"
#include "logger.h"
#include "cmdimp.h"

<<<<<<< HEAD
int CommandMap::exec_command(cmdargs &cmd, std::string raw_command) {
  // Translate to default command name and check if it's valid
  std::string command = aliases[raw_command];
  command_info_map::iterator it = info_map.find(command);
  if(it == info_map.end()) {
    Logger::fatal << "Unrecognized command: " << raw_command << Logger::endl;
    return -1;
  }

  // run the command and return its result
  command_info ci = it->second;
  return ci.helper_fn(cmd);
}

CommandMap init_autolab_command_map() {
  command_alias_map aliases;
  aliases["status"] = "status";
  aliases["download"] = "download";
  aliases["submit"] = "submit";
  aliases["courses"] = "courses";
  aliases["assessments"] = "assessments";
  aliases["asmts"] = "assessments";
  aliases["problems"] = "problems";
  aliases["scores"] = "scores";
  aliases["feedback"] = "feedback";
  aliases["enroll"] = "enroll";

  command_info_map info_map {
    // general commands
    {"status",     {"status              Show status of the local assessment",     &show_status,      false}},
    {"download",   {"download            Download files needed for an assessment", &download_asmt,    false}},
    {"submit",     {"submit              Submit a file to an assessment",          &submit_asmt,      false}},
    {"courses",    {"courses             List all courses",                        &show_courses,     false}},
    {"assessments",{"assessments/asmts   List all assessments of a course",        &show_assessments, false}},
    {"problems",   {"problems            List all problems in an assessment",      &show_problems,    false}},
    {"scores",     {"scores              Show scores got on an assessment",        &show_scores,      false}},
    {"feedback",   {"feedback            Show feedback on a submission",           &show_feedback,    false}},
    // instructor commands
    {"enroll",     {"enroll              Manage users affiliated with a course",   &manage_enrolls,   true}}
  };

  CommandMap command_map = CommandMap();
  command_map.aliases = aliases;
  command_map.info_map = info_map;

=======
std::string CommandMap::get_usage(std::string command) {
  // Check if command exists?
  std::string result;
  try {
    result = info_map[command]->usage;
  }
  catch(int e) {
    Logger::info << "get_usage(" << command << ") throws error" << e;
    result = "Error: usage not found";
  }
  return result;
}

int CommandMap::exec_command(cmdargs &cmd, std::string command) {
  int result;
  try {
    result =  info_map[command]->helper_fn(cmd);
  }
  catch(int e) {
    Logger::info << "exec_command(" << command << ") throws error" << e;
    result = -1;
  }
  return result;
}

CommandMap init_autolab_command_map() {

  std::vector<std::string> valid_commands = {"status", "download", "submit",
                                             "courses", "assessments", "asmts",
                                             "problems", "scores", "feedback",
                                             "enroll"};

  // There's gotta be a better way to do this
  std::map<std::string, command_info *> *info_map = (std::map<std::string, command_info *> *)malloc(sizeof(std::map<std::string, command_info *>));

  // Setup status command_info

  // We need to malloc this so that the memory isn't cleaned up after this
  // function finishes execution. Problem is, we now need a function to free
  // All of these.
  command_info *status_ci = (command_info *)malloc(sizeof(command_info));
  status_ci->usage = "status              Show status of the local assessment";
  status_ci->helper_fn = &show_status;
  status_ci->instructor_command = false;
  (*info_map)["status"] = status_ci;

  // Setup download command_info
  command_info *download_ci = (command_info *)malloc(sizeof(command_info));
  download_ci->usage = "download            Download files needed for an assessment";
  download_ci->helper_fn = &download_asmt;
  download_ci->instructor_command = false;
  (*info_map)["download"] = download_ci;

  // Setup submit command_info
  command_info *submit_ci = (command_info *)malloc(sizeof(command_info));
  submit_ci->usage = "submit              Submit a file to an assessment";
  submit_ci->helper_fn = &submit_asmt;
  submit_ci->instructor_command = false;
  (*info_map)["submit"] = submit_ci;

  // Setup courses command_info
  command_info *courses_ci = (command_info *)malloc(sizeof(command_info));
  courses_ci->usage = "courses             List all courses";
  courses_ci->helper_fn = &show_courses;
  courses_ci->instructor_command = false;
  (*info_map)["courses"] = courses_ci;

  // Setup assessments command_info
  command_info *assessments_ci = (command_info *)malloc(sizeof(command_info));
  assessments_ci->usage = "assessments/asmts   List all assessments of a course";
  assessments_ci->helper_fn = &show_assessments;
  assessments_ci->instructor_command = false;
  (*info_map)["assessments"] = assessments_ci;

  // Setup asmts command_info

  // Since the commands are aliased, they should have the same command info.
  command_info *asmts_ci = assessments_ci;
  (*info_map)["asmts"] = asmts_ci;

  // Setup problems command_info
  command_info *problems_ci = (command_info *)malloc(sizeof(command_info));
  problems_ci->usage = "problems            List all problems in an assessment";
  problems_ci->helper_fn = &show_problems;
  problems_ci->instructor_command = false;
  (*info_map)["problems"] = problems_ci;

  // Setup scores command_info
  command_info *scores_ci = (command_info *)malloc(sizeof(command_info));
  scores_ci->usage = "scores              Show scores got on an assessment";
  scores_ci->helper_fn = &show_scores;
  scores_ci->instructor_command = false;
  (*info_map)["scores"] = scores_ci;

  // Setup feedback command_info
  command_info *feedback_ci = (command_info *)malloc(sizeof(command_info));
  feedback_ci->usage = "feedback            Show feedback on a submission";
  feedback_ci->helper_fn = &show_feedback;
  feedback_ci->instructor_command = false;
  (*info_map)["feedback"] = feedback_ci;

  // Setup scores command_info
  command_info *enroll_ci = (command_info *)malloc(sizeof(command_info));
  enroll_ci->usage = "enroll              Manage users affiliated with a course";
  enroll_ci->helper_fn = &manage_enrolls;
  enroll_ci->instructor_command = true;
  (*info_map)["enroll"] = enroll_ci;

  CommandMap command_map = CommandMap();
  command_map.commands = valid_commands;
  command_map.info_map = *info_map;

>>>>>>> implemented init_autolab_command_map
  return command_map;
}
