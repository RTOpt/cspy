from networkx import DiGraph
from numpy import array

from cspy.algorithms.tabu import Tabu
from cspy.algorithms.bidirectional import BiDirectional

from utils import TestingBase


class TestsIssue17(TestingBase):
    """
    Tests for issue #17
    https://github.com/torressa/cspy/issues/17
    """

    def setUp(self):
        # Create simple digraph with appropriate attributes
        self.G = DiGraph(directed=True, n_res=2)
        self.G.add_edge("Source", 1, weight=3, res_cost=array([1, 1]))
        self.G.add_edge("Source", 2, weight=0, res_cost=array([1, 1]))
        self.G.add_edge(1, 2, weight=-1, res_cost=array([1, 1]))
        self.G.add_edge(1, 4, weight=5, res_cost=array([1, 1]))
        self.G.add_edge(2, 3, weight=3, res_cost=array([1, 1]))
        self.G.add_edge(3, 1, weight=1, res_cost=array([1, 1]))
        self.G.add_edge(2, 5, weight=-1, res_cost=array([1, 1]))
        self.G.add_edge(5, "Sink", weight=2, res_cost=array([1, 1]))
        self.G.add_edge(5, 4, weight=-1, res_cost=array([1, 1]))
        self.G.add_edge(4, 2, weight=3, res_cost=array([1, 1]))
        self.G.add_edge(4, "Sink", weight=3, res_cost=array([1, 1]))
        # Maximum and minimum resource arrays
        self.max_res, self.min_res = [len(self.G.nodes()), 6], [0, 0]
        # Expected results
        self.result_path = ["Source", 2, 5, "Sink"]
        self.total_cost = 1
        self.consumed_resources = [3, 3]

    def test_bidirectional(self):
        alg = BiDirectional(self.G, self.max_res, self.min_res, elementary=True)
        alg.run()
        self.assertEqual(alg.path, self.result_path)
        self.assertEqual(alg.total_cost, self.total_cost)
        self.assertTrue(alg.consumed_resources == self.consumed_resources)
        self.assertTrue(all(e in self.G.edges() for e in zip(alg.path, alg.path[1:])))

    def test_bidirectional_forward(self):
        alg = BiDirectional(
            self.G, self.max_res, self.min_res, direction="forward", elementary=True
        )
        alg.run()
        self.check_result(
            alg, self.result_path, self.total_cost, self.consumed_resources
        )
        self.assertTrue(all(e in self.G.edges() for e in zip(alg.path, alg.path[1:])))

    def test_bidirectional_backward(self):
        alg = BiDirectional(
            self.G, self.max_res, self.min_res, direction="backward", elementary=True
        )
        alg.run()
        self.check_result(
            alg, self.result_path, self.total_cost, self.consumed_resources
        )
        self.assertTrue(all(e in self.G.edges() for e in zip(alg.path, alg.path[1:])))

    def test_tabu(self):
        alg = Tabu(self.G, self.max_res, self.min_res)
        alg.run()
        self.check_result(alg, ["Source", 2, 5, 4, "Sink"], 1, [4, 4])
        self.assertTrue(all(e in self.G.edges() for e in zip(alg.path, alg.path[1:])))
