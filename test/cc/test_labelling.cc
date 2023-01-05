#include <algorithm> // make_heap, push_heap
#include <memory>    // make_unique

#include "gtest/gtest.h"
#include "src/cc/bidirectional.h"
#include "src/cc/digraph.h"
#include "src/cc/labelling.h"

namespace labelling {

class TestLabelling : public ::testing::Test {
 protected:
  double                                 weight     = 10.0;
  bidirectional::Vertex                  node       = {1, 1}; // B
  bidirectional::Vertex                  other_node = {2, 2}; // C
  std::vector<double>                    res        = {6.0, 5.0};
  std::vector<int>                       path       = {1};
  std::vector<double>                    max_res    = {20.0, 20.0};
  std::vector<double>                    min_res    = {0.0, 0.0};
  std::unique_ptr<bidirectional::Params> params_ptr =
      std::make_unique<bidirectional::Params>();
};

TEST_F(TestLabelling, testDominance) {
  const Label         label(weight, node, res, path, params_ptr.get());
  std::vector<double> res2 = {6.0, -3.0};
  const Label         label2(weight, node, res2, path, params_ptr.get());
  const Label         label3(weight, node, res2, path, params_ptr.get());

  ASSERT_TRUE(label2.checkDominance(label, bidirectional::FWD));
  ASSERT_FALSE(label.checkDominance(label2, bidirectional::FWD));
  ASSERT_TRUE(label3.checkDominance(label, bidirectional::BWD));
  ASSERT_FALSE(label3.checkDominance(label2, bidirectional::BWD));
}

TEST_F(TestLabelling, testDominanceElementary) {
  params_ptr->elementary = true;
  // L1
  Label label(weight, node, res, path, params_ptr.get());
  label.unreachable_nodes = std::set<int>({1, 2, 3});
  // L2
  std::vector<double> res2 = {6.0, 4.0};
  Label               label2(weight, node, res2, path, params_ptr.get());
  // Unrelated U2
  label2.unreachable_nodes = std::set<int>({4, 5, 6});

  // L2 dominates (due to resources)
  ASSERT_FALSE(label.checkDominance(label2, bidirectional::FWD));
  ASSERT_FALSE(label2.checkDominance(label, bidirectional::FWD));

  // Make U2 \subset U1
  label2.unreachable_nodes = std::set<int>({1, 2});
  // L2 now dominates L1 now as U2 \subset U1
  ASSERT_FALSE(label.checkDominance(label2, bidirectional::FWD));
  ASSERT_TRUE(label2.checkDominance(label, bidirectional::FWD));

  // Make U1 \subset U2
  label2.unreachable_nodes = std::set<int>({1, 2, 3, 4});
  // Neither dominate. As U2 is not a \subset U1
  ASSERT_FALSE(label.checkDominance(label2, bidirectional::FWD));
  ASSERT_FALSE(label2.checkDominance(label, bidirectional::FWD));

  // Make U1 = U2
  label2.unreachable_nodes = std::set<int>({1, 2, 3});
  // L2 dominates as tie breaker because of resources. If we don't check for
  // equality in checkDominance, neither would dominate.
  ASSERT_FALSE(label.checkDominance(label2, bidirectional::FWD));
  ASSERT_TRUE(label2.checkDominance(label, bidirectional::FWD));
}

TEST_F(TestLabelling, testDominanceElementaryIssue94) {
  params_ptr->elementary = true;
  // L1
  std::vector<int> path1{0, 2, 3};
  Label            label(6.0, node, {2.0}, path1, params_ptr.get());
  label.unreachable_nodes = std::set<int>({0, 2, 3});
  // L2
  std::vector<int> path2{0, 1, 3};
  Label            label2(11.0, node, {2.0}, path2, params_ptr.get());
  label2.unreachable_nodes = std::set<int>({0, 1, 3});

  ASSERT_FALSE(label2.checkDominance(label, bidirectional::FWD));
  ASSERT_FALSE(label.checkDominance(label2, bidirectional::FWD));
}

TEST_F(TestLabelling, testThreshold) {
  const Label  label(weight, node, res, path, params_ptr.get());
  const double threshold1 = 11.0;
  const double threshold2 = 0.0;

  ASSERT_TRUE(label.checkThreshold(threshold1));
  ASSERT_FALSE(label.checkThreshold(threshold2));
}

TEST_F(TestLabelling, testStPath) {
  const Label label(weight, node, res, path, params_ptr.get());

  std::vector<int> path2{0, 10};
  const Label      label2(weight, node, res, path2, params_ptr.get());

  ASSERT_FALSE(label.checkStPath(0, 10));
  ASSERT_TRUE(label2.checkStPath(0, 10));
}

TEST_F(TestLabelling, testFeasibility) {
  const Label               label(weight, node, res, path, params_ptr.get());
  const std::vector<double> max_res = {10.0, 10.0};
  const std::vector<double> min_res = {0.0, 0.0};

  ASSERT_TRUE(label.checkFeasibility(max_res, min_res));
  ASSERT_FALSE(label.checkFeasibility(min_res, max_res));
}

TEST_F(TestLabelling, testFeasibilitySoft) {
  const Label               label(weight, node, res, path, params_ptr.get());
  const std::vector<double> min_res = {6.0, 10.0};

  // Soft passes as critical resource is at index 0 and res[0] = 6.0 <=
  // min_res[0] = 6.0, and index 1 is not checked as bound is not <= 0
  ASSERT_TRUE(label.checkFeasibility(max_res, min_res, true));
  // Hard fails as res[1] = 5.0 is not >= min_res[1] = 10.0
  ASSERT_FALSE(label.checkFeasibility(max_res, min_res));
}

TEST_F(TestLabelling, testExtendForward) {
  Label label(weight, node, res, path, params_ptr.get());
  auto  labels                         = std::make_unique<std::vector<Label>>();
  const bidirectional::AdjVertex adj_v = {other_node, weight, res};

  std::make_heap(labels->begin(), labels->end(), std::greater<>{});
  labels->push_back(label);
  Label new_label = label.extend(adj_v, bidirectional::FWD, max_res, min_res);
  labels->push_back(new_label);
  std::push_heap(labels->begin(), labels->end(), std::greater<>{});

  ASSERT_EQ(labels->size(), 2);
  // Should return labels in decreasing order of the monotone resource
  Label next_label = getNextLabel(labels.get(), bidirectional::FWD);
  ASSERT_EQ(labels->size(), 1);
  ASSERT_EQ(next_label.resource_consumption[0], 6);
  ASSERT_EQ(next_label.vertex.lemon_id, 1);
  Label last_label = getNextLabel(labels.get(), bidirectional::FWD);
  ASSERT_EQ(labels->size(), 0);
  ASSERT_EQ(last_label.resource_consumption[0], 12);
  ASSERT_EQ(last_label.vertex.lemon_id, 2);
}

TEST_F(TestLabelling, testExtendBackward) {
  Label label(weight, node, res, path, params_ptr.get());
  auto  labels = std::make_unique<std::vector<Label>>();

  const bidirectional::AdjVertex adj_v = {other_node, weight, res};
  // Max-heap
  std::make_heap(labels->begin(), labels->end());
  // Insert current label
  labels->push_back(label);
  // extend current label
  Label new_label = label.extend(adj_v, bidirectional::BWD, max_res, min_res);
  labels->push_back(new_label);
  std::push_heap(labels->begin(), labels->end());

  // Should return labels in increasing order of the monotone resource
  ASSERT_EQ(labels->size(), 2);
  Label next_label = getNextLabel(labels.get(), bidirectional::BWD);
  ASSERT_EQ(next_label.resource_consumption[0], 6);
  ASSERT_EQ(labels->size(), 1);
  Label last_label = getNextLabel(labels.get(), bidirectional::BWD);
  ASSERT_EQ(labels->size(), 0);
  ASSERT_EQ(last_label.resource_consumption[0], 0);
  ASSERT_EQ(last_label.vertex.lemon_id, 2);
}

TEST_F(TestLabelling, testRunDominanceForward) {
  std::vector<double> res2 = {3.0, -3.0};
  std::vector<double> res3 = {1.0, -3.0};
  const Label         label1(weight, node, res, path, params_ptr.get());
  const Label         label2(weight, node, res2, path, params_ptr.get());
  const Label         label3(weight, node, res3, path, params_ptr.get());

  auto labels = std::make_unique<std::vector<Label>>();

  // Insert labels
  labels->push_back(label2);
  labels->push_back(label1);
  labels->push_back(label3);

  runDominanceEff(labels.get(), label3, bidirectional::FWD, false);
  ASSERT_EQ(labels->size(), 1);
  ASSERT_EQ((*labels)[0], label3);
}

TEST_F(TestLabelling, testRunDominanceBackward) {
  std::vector<double> res2 = {3.0, res[1]};
  std::vector<double> res3 = {7.0, res[1]};
  const Label         label1(weight, node, res, path, params_ptr.get());
  const Label         label2(weight, node, res2, path, params_ptr.get());
  const Label         label3(weight, node, res3, path, params_ptr.get());

  auto labels = std::make_unique<std::vector<Label>>();

  labels->push_back(label1);
  labels->push_back(label2);
  labels->push_back(label3);

  runDominanceEff(labels.get(), label3, bidirectional::BWD, false);
  ASSERT_EQ(labels->size(), 1);
  ASSERT_EQ((*labels)[0], label3);
}

TEST_F(TestLabelling, testTwoCycleExtension) {
  std::vector<int> path1{0, 6, 7, 8};
  std::vector<int> path2{0, 1, 3, 4, 8};

  Label label1(-270.0, node, {2.0}, path1, params_ptr.get());
  Label label2(-260.0, node, {2.0}, path2, params_ptr.get());

  ASSERT_TRUE(label1.checkDominance(label2, bidirectional::FWD));

  params_ptr->two_cycle_elimination = true;
  ASSERT_FALSE(label1.checkDominance(label2, bidirectional::FWD));
}

} // namespace labelling

// namespace labelling
