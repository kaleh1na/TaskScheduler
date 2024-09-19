#include <iostream>
#include <vector>

class Any {
 private:
  struct AnyConcept {
    virtual AnyConcept* GetCopy() const = 0;
    virtual ~AnyConcept() = default;
  };

  template <typename T>
  struct AnyModel : public AnyConcept {
    T value;
    AnyModel(const T& value) : value(value) {}
    ~AnyModel() override = default;
    AnyConcept* GetCopy() const override { return new AnyModel<T>(value); }
  };

  AnyConcept* ptr_;

 public:
  template <typename T>
  friend T Any_cast(Any& a) {
    auto* p = dynamic_cast<Any::AnyModel<std::remove_reference_t<T>>*>(a.ptr_);
    if (!p) {
      throw std::bad_cast();
    } else {
      return (*p).value;
    }
  }

  Any() : ptr_(nullptr) {}

  template <typename T>
  Any(const T& value) : ptr_(new AnyModel<T>(value)) {}

  Any(const Any& other) : ptr_(other.ptr_->GetCopy()) {}

  template <typename T>
  Any& operator=(const T& other) {
    if (ptr_ != nullptr) {
      delete ptr_;
    }
    ptr_ = new AnyModel<T>(other);
    return *this;
  }

  Any& operator=(const Any& other) {
    if (ptr_ == other.ptr_) {
      return *this;
    }
    if (ptr_ != nullptr) {
      delete ptr_;
    }
    ptr_ = other.ptr_->GetCopy();
    return *this;
  }

  ~Any() {
    if (ptr_) delete ptr_;
  }
};

class Task;

template <typename T>
class FutureResult {
 private:
  Task* task_;

 public:
  friend T Get(const FutureResult<T>& t) {
    try {
      return t.task_->template GetResult<T>();
    } catch (const std::bad_cast&) {
      throw std::bad_cast();
    }
  }

  FutureResult(Task* task) : task_(task){};
};

template <typename T>
T Get(const T& t) {
  return t;
}

class Task {
 private:
 public:
  struct TaskConcept {
    virtual void DoTask() = 0;
    virtual TaskConcept* GetCopy() const = 0;
    virtual ~TaskConcept() = default;
    Any result;
  };

  template <typename F, typename A, typename B>
  struct TaskTwoArgsModel : public TaskConcept {
    F f;
    A a;
    B b;
    TaskTwoArgsModel(const F& f, const A& a, const B& b) : f(f), a(a), b(b) {}
    void DoTask() override { result = f(Get(a), Get(b)); }
    TaskConcept* GetCopy() const override {
      return new TaskTwoArgsModel<F, A, B>(f, a, b);
    }
    ~TaskTwoArgsModel() override = default;
  };

  template <typename F, typename A>
  struct TaskOneArgModel : public TaskConcept {
    F f;
    A a;
    TaskOneArgModel(const F& f, const A& a) : f(f), a(a) {}
    void DoTask() override { result = f(Get(a)); }
    TaskConcept* GetCopy() const override {
      return new TaskOneArgModel<F, A>(f, a);
    }
    ~TaskOneArgModel() override = default;
  };

  template <typename F>
  struct TaskZeroArgsModel : public TaskConcept {
    F f;
    TaskZeroArgsModel(const F& f) : f(f) {}
    void DoTask() override { result = f(); }
    TaskConcept* GetCopy() const override {
      return new TaskZeroArgsModel<F>(f);
    }
    ~TaskZeroArgsModel() override = default;
  };

  TaskConcept* task_;
  bool is_done_ = false;

 public:
  template <typename F, typename A, typename B>
  Task(const F& f, const A& a, const B& b)
      : task_(new TaskTwoArgsModel<F, A, B>(f, a, b)) {}

  template <typename F, typename A>
  Task(const F& f, const A& a) : task_(new TaskOneArgModel<F, A>(f, a)) {}

  template <typename F>
  Task(const F& f) : task_(new TaskZeroArgsModel<F>(f)) {}

  Task(const Task& other) : task_(other.task_->GetCopy()) {}

  Task& operator=(const Task& other) {
    if (task_ == other.task_) {
      return *this;
    }
    delete task_;
    task_ = other.task_->GetCopy();
    return *this;
  }

  ~Task() { delete task_; }

  template <typename T>
  T GetResult() {
    DoTask();
    try {
      return Any_cast<T>(task_->result);
    } catch (const std::bad_cast&) {
      throw std::bad_cast();
    }
  }

  void DoTask() {
    if (!is_done_) {
      task_->DoTask();
    }
    is_done_ = true;
  }
};

class TaskScheduler {
 private:
  std::vector<Task*> tasks_;

 public:
  TaskScheduler() {}

  TaskScheduler(const TaskScheduler& other) {
    for (int i = 0; i < other.tasks_.size(); ++i) {
      tasks_.push_back(new Task(*other.tasks_[i]));
    }
  }

  TaskScheduler& operator=(const TaskScheduler& other) {
    if (*this == other) {
      return *this;
    }
    for (int i = 0; i < tasks_.size(); ++i) {
      delete tasks_[i];
    }
    for (int i = 0; i < other.tasks_.size(); ++i) {
      tasks_.push_back(new Task(*other.tasks_[i]));
    }
    return *this;
  }

  ~TaskScheduler() {
    for (int i = 0; i < tasks_.size(); ++i) {
      delete tasks_[i];
    }
  }

  bool operator==(const TaskScheduler& other) {
    if (tasks_.size() != other.tasks_.size()) {
      return false;
    }
    for (int i = 0; i < tasks_.size(); ++i) {
      if (tasks_[i] != other.tasks_[i]) {
        return false;
      }
    }
    return true;
  }

  template <typename F, typename A, typename B>
  Task* Add(const F& f, const A& a, const B& b) {
    tasks_.push_back(new Task(f, a, b));
    return tasks_.back();
  }

  template <typename F, typename A>
  Task* Add(const F& f, const A& a) {
    tasks_.push_back(new Task(f, a));
    return tasks_.back();
  }

  template <typename F>
  Task* Add(const F& f) {
    tasks_.push_back(new Task(f));
    return tasks_.back();
  }

  template <typename T>
  FutureResult<T> GetFutureResult(Task* t) {
    return FutureResult<T>(t);
  }

  template <typename T>
  T GetResult(Task* t) const {
    try {
      return t->GetResult<T>();
    } catch (const std::bad_cast&) {
      throw std::bad_cast();
    }
  }

  void ExecuteAll() {
    for (int i = 0; i < tasks_.size(); ++i) {
      tasks_[i]->DoTask();
    }
  }
};
