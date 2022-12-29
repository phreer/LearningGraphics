#include <string>

class Application {
 public:
  Application(const std::string &app_name): is_running_(false), app_name_(app_name) {}
  virtual ~Application() {};
  bool is_running() const { return is_running_; }
  std::string app_name() const { return app_name_; }
  void Run() {
    if (is_running()) {
      fprintf(stderr, "application %s is running already", app_name().c_str());
      exit(EXIT_FAILURE);
    }
    is_running_ = true;
    StartUp_();
    Render_();
    ShutDown_();
    is_running_ = false;
  }

 private:
  virtual void StartUp_() = 0;
  virtual void ShutDown_() = 0;
  virtual void Render_() = 0;

  bool is_running_;
  std::string app_name_;
};
