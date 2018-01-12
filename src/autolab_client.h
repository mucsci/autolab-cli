#ifndef AUTOLAB_CLIENT_H
#define AUTOLAB_CLIENT_H

#include <ostream>
#include <fstream>
#include <vector>

#include <curl/curl.h>

#include <rapidjson/document.h>

class AutolabClient {
public:
  AutolabClient(const std::string &id, const std::string &st, const std::string &ru);

  // setters and getters
  void set_tokens(std::string at, std::string rt);
  const std::string get_access_token() { return access_token; }
  const std::string get_refresh_token() { return refresh_token; }

  /* oauth-related */
  void device_flow_init(std::string &user_code, std::string &verification_uri);
  int device_flow_authorize(size_t timeout);

  class HTTPException: public std::exception {
  private:
    std::string msg;
  public:
    explicit HTTPException(const char *m) : msg(m) {}
    virtual const char* what() const throw() {
        return msg.c_str();
    }
  };
  class InvalidTokenException: public std::exception {
  public:
    virtual const char* what() const throw() {
        return "The provided access token is invalid and the refresh operation failed.";
    }
  };

  // keeps track of state and config for the current request.
  struct request_state {
    bool is_download;
    std::string suggested_filename;
    std::string download_dir;
    std::string string_output;
    std::ofstream file_output;
    long response_code;

    request_state() :
      is_download(false) {}
    request_state(std::string dir, std::string name_hint) :
      download_dir(dir), suggested_filename(name_hint), is_download(false) {}

    void reset() {
      is_download = false;
      string_output.clear();
    }

    bool consider_download() {
      return download_dir.length() > 0;
    }
  };

  /* REST interface methods */
  void get_user_info(rapidjson::Document &result);
  void get_courses(rapidjson::Document &result);
  void get_assessments(rapidjson::Document &result, std::string course_name);
  void download_handout(std::string filename, std::string course_name, std::string asmt_name);

private:
  // initializes curl interface. Must be called before anything else.
  static int curl_ready;
  static int init_curl();

  enum HttpMethod {GET, POST};

  // represents the parameters used in a HTTP request.
  // can be used for both GET and POST requests
  struct request_param {
    std::string key;
    std::string value;
    char *escaped_value;

    request_param(std::string k, std::string v) :
      key(k), value(v), escaped_value(nullptr) {}
  };
  typedef std::vector<request_param> param_list;

  std::string construct_params(CURL *curl, param_list &params);
  void free_params(param_list &params);

  // private instance vars
  int api_version;

  std::string client_id;
  std::string client_secret;
  std::string redirect_uri;
  std::string access_token;
  std::string refresh_token;
  std::string device_flow_device_code;
  std::string device_flow_user_code;

  // perform HTTP request and return result, default method is GET.
  long raw_request(request_state *rstate, const std::string &path, param_list &params, HttpMethod method);
  long raw_request_optional_refresh(request_state *rstate, const std::string &path, param_list &params, HttpMethod method, bool refresh);
  long download_request(const std::string &download_dir, const std::string &suggested_filename, const std::string &path, param_list &params, HttpMethod method, bool refresh);
  long json_request(rapidjson::Document &response, const std::string &path, param_list &params, HttpMethod method, bool refresh);

  void clear_device_flow_strings();

  bool save_tokens_from_response(rapidjson::Document &response);
  bool get_token_from_authorization_code(std::string authorization_code);
public:
  bool perform_token_refresh();

  bool document_has_error(request_state *rstate, const std::string &error_msg);
  void init_regular_path(std::string &path);
  void init_regular_params(param_list &params);
};

#endif /* AUTOLAB_CLIENT_H */