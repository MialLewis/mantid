// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TaskBasedAlgorithm.h"
// #include "MantidAPI/AnalysisDataService.h"

namespace {
class ToyTaskBasedAlg : public Mantid::API::TaskBasedAlgorithm<ToyTaskBasedAlg> {
public:
  const std::string name() const override { return "ToyTaskBasedAlg"; };
  const std::string summary() const override { return "Toy alg for task based algorithm test"; }
  int version() const override { return 1; };
  void init() override {};
  void exec() override {};

  // Possible orders tested:
  // 1) A, B, C, D
  // 2) A, C, B, D
  // 3) A, B, D

  class TaskA final : public AlgorithmTask {
  public:
    explicit TaskA(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskA") { setExpectedOutputs({"TaskAOutput"}); }
    void executeImpl() override {};
  };

  class TaskB final : public AlgorithmTask {
  public:
    explicit TaskB(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskB") {
      setExpectedOutputs({"TaskBOutput1", "TaskBOutput2"});
      setDependantTask("TaskA", "TaskAOutput", "InputWorkspace");
      auto depSet = addDependantTaskSet();
      setDependantTask("TaskC", "TaskCOutput", "InputWorkspace", depSet);
    }
    void executeImpl() override {};
  };

  class TaskC final : public AlgorithmTask {
  public:
    explicit TaskC(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskC") {
      setExpectedOutputs({"TaskCOutput"});
      setDependantTask("TaskB", "TaskBOutput1", "InputWorkspace");
      auto depSet = addDependantTaskSet();
      setDependantTask("TaskA", "TaskAOutput", "InputWorkspace", depSet);
    }
    void executeImpl() override {};
  };

  class TaskD final : public AlgorithmTask {
  public:
    explicit TaskD(ToyTaskBasedAlg *parent) : AlgorithmTask(parent, "TaskD") {
      setExpectedOutputs({"TaskDOutput1", "TaskDOutput2"});
      setDependantTask("TaskC", "TaskCOutput", "InputWorkspace");
      auto depSet = addDependantTaskSet();
      setDependantTask("TaskB", "TaskBOutput2", "InputWorkspace", depSet);
    }
    void executeImpl() override {};
  };
};
} // namespace

class TaskBasedAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TaskBasedAlgorithmTest *createSuite() { return new TaskBasedAlgorithmTest(); }
  static void destroySuite(TaskBasedAlgorithmTest *suite) { delete suite; }

  TaskBasedAlgorithmTest() {
    Mantid::API::FrameworkManager::Instance();
    // AnalysisDataService::Instance();
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyTaskBasedAlg>();
  }

  ~TaskBasedAlgorithmTest() override { Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm", 1); }

  void setUp() override {} // AnalysisDataService::Instance().clear(); }

  void testAlgorithm() {}

private:
  ToyTaskBasedAlg alg;
};
