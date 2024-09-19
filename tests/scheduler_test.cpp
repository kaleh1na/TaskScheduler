#include <gtest/gtest.h>

#include <lib/scheduler.cpp>
#include <string>

int Pow(int x, int p) {
  int res = 1;
  for (int i = 0; i < p; ++i) {
    res *= x;
  }
  return res;
}

std::string operator*(const std::string& s, int a) {
  std::string ans;
  for (int i = 0; i < a; i++) {
    for (int j = 0; j < s.size(); ++j) {
      ans.push_back(s[j]);
    }
  }
  return ans;
}

TEST(SchedulerTestSuit, AnyTest) {
  int c = 5;
  Any a = c;
  ASSERT_EQ(Any_cast<int&>(a), 5);
  std::string b = "abc";
  a = b;
  ASSERT_EQ(Any_cast<std::string&>(a), "abc");
  Any d = a;
  ASSERT_EQ(Any_cast<std::string&>(d), "abc");
  a = 100;
  ASSERT_EQ(Any_cast<std::string&>(d), "abc");
  a = a;
  ASSERT_EQ(Any_cast<int&>(a), 100);
}

TEST(SchedulerTestSuit, GetResultFromFutureResultTest) {
  int a = 1;
  int b = -2;
  int c = 3;

  TaskScheduler scheduler;

  auto id1 = scheduler.Add([](int a, int c) { return -4 * a * c; }, a, c);
  auto id2 = scheduler.Add([](int a, int b) { return -4 * a * b; }, a, b);
  auto id3 = scheduler.Add([](int b, int c) { return b + c; },
                           scheduler.GetFutureResult<int>(id2),
                           scheduler.GetFutureResult<int>(id1));

  ASSERT_EQ(scheduler.GetResult<int>(id3), -4);
}

TEST(SchedulerTestSuit, CopyTest) {
  int a = 1;
  int b = -2;
  int c = 3;

  TaskScheduler scheduler1;

  auto id1 = scheduler1.Add([](int a, int c) { return -4 * a * c; }, a, c);

  TaskScheduler scheduler2 = scheduler1;
  auto id2 = scheduler2.Add([]() { return 100; });
  scheduler2.ExecuteAll();

  ASSERT_EQ(scheduler2.GetResult<int>(id2), 100);
}

TEST(SchedulerTestSuit, ReadMeTest) {
  double a = 2;
  double b = 10;
  double c = 8;

  TaskScheduler scheduler;

  auto id1 = scheduler.Add([](double a, double c) { return -4 * a * c; }, a, c);
  auto id2 = scheduler.Add([](double b, double v) { return b * b + v; }, b,
                           scheduler.GetFutureResult<double>(id1));
  auto id3 = scheduler.Add([](double b, double d) { return -b + std::sqrt(d); },
                           b, scheduler.GetFutureResult<double>(id2));
  auto id4 = scheduler.Add([](double b, double d) { return -b - std::sqrt(d); },
                           b, scheduler.GetFutureResult<double>(id2));
  auto id5 = scheduler.Add([](double a, double v) { return v / (2 * a); }, a,
                           scheduler.GetFutureResult<double>(id3));
  auto id6 = scheduler.Add([](double a, double v) { return v / (2 * a); }, a,
                           scheduler.GetFutureResult<double>(id4));

  scheduler.ExecuteAll();

  ASSERT_EQ(scheduler.GetResult<double>(id5), -1);
  ASSERT_EQ(scheduler.GetResult<double>(id6), -4);
}

TEST(SchedulerTestSuit, ConcatenationTest) {
  std::string s1 = "Hello";
  std::string s2 = " ";
  std::string s3 = "World";
  std::string s4 = "!";

  TaskScheduler scheduler;

  auto id1 = scheduler.Add(
      [](const std::string& a, const std::string& b) { return a + b; }, s1, s2);
  auto id2 = scheduler.Add(
      [](const std::string& a, const std::string& b) { return a + b; }, s3, s4);
  auto id3 = scheduler.Add(
      [](const std::string& a, const std::string& b) { return a + b; },
      scheduler.GetFutureResult<std::string>(id1),
      scheduler.GetFutureResult<std::string>(id2));

  ASSERT_EQ(scheduler.GetResult<std::string>(id3), "Hello World!");
}

TEST(SchedulerTestSuit, StringAndIntTest) {
  std::string s = "again";
  int a = 10;

  TaskScheduler scheduler;

  auto id1 = scheduler.Add([]() { return 4; });
  auto id2 = scheduler.Add([](int a, int b) { return a / b; }, a,
                           scheduler.GetFutureResult<int>(id1));
  auto id3 = scheduler.Add([](int a) { return a * 3; },
                           scheduler.GetFutureResult<int>(id2));
  auto id4 = scheduler.Add([](const std::string& s, int a) { return s * a; }, s,
                           scheduler.GetFutureResult<int>(id3));

  ASSERT_EQ(scheduler.GetResult<std::string>(id4),
            "againagainagainagainagainagain");
}

TEST(SchedulerTestSuit, FindCylinderVolumeAndSquareTest) {
  double h = 10;
  double r = 2.5;

  TaskScheduler scheduler;

  auto id1 = scheduler.Add([](double r) { return r * r * 3.14; }, r);
  auto id2 = scheduler.Add([](double r) { return 2 * 3.14 * r; }, r);
  auto id3 = scheduler.Add([](double s, double h) { return s * h; },
                           scheduler.GetFutureResult<double>(id1), h);
  auto id4 = scheduler.Add([](double l, double h) { return l * h; },
                           scheduler.GetFutureResult<double>(id2), h);
  auto id5 = scheduler.Add([](double s1, double s2) { return 2 * s1 + s2; },
                           scheduler.GetFutureResult<double>(id1),
                           scheduler.GetFutureResult<double>(id4));

  ASSERT_EQ(scheduler.GetResult<double>(id3), 196.25);
  ASSERT_EQ(scheduler.GetResult<double>(id5), 196.25);
}

TEST(SchedulerTestSuit, DefiniteIntegralTest) {
  // y = cx^p
  int c = 9;
  int p = 2;
  int a = -5;
  int b = 10;

  TaskScheduler scheduler;

  auto id1 = scheduler.Add([](int p) { return p + 1; }, p);
  auto id2 = scheduler.Add([](int c, int p) { return c / p; }, c,
                           scheduler.GetFutureResult<int>(id1));
  auto id3 = scheduler.Add([](int x, int p) { return Pow(x, p); }, b,
                           scheduler.GetFutureResult<int>(id1));
  auto id4 = scheduler.Add([](int x, int c) { return x * c; },
                           scheduler.GetFutureResult<int>(id3),
                           scheduler.GetFutureResult<int>(id2));
  auto id5 = scheduler.Add([](int x, int p) { return Pow(x, p); }, a,
                           scheduler.GetFutureResult<int>(id1));
  auto id6 = scheduler.Add([](int x, int c) { return x * c; },
                           scheduler.GetFutureResult<int>(id5),
                           scheduler.GetFutureResult<int>(id2));
  auto id7 = scheduler.Add([](int b, int a) { return b - a; },
                           scheduler.GetFutureResult<int>(id4),
                           scheduler.GetFutureResult<int>(id6));

  ASSERT_EQ(scheduler.GetResult<int>(id7), 3375);
}