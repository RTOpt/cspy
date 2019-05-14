from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from operator import add, sub


class Label(object):
    '''Label object that allows comparison.
    PARAMS:
        weight :: float, cumulative edge weight
        node   :: string, name of last node visited
        res    :: list, cumulative edge resource consumption
        path   :: list, of all nodes in the path'''

    def __init__(self, weight, node, res, path):
        self.weight = weight  # type: float
        self.node = node  # type: str
        self.res = res  # type: list[float]
        self.path = path  # type: list[str]

    def __repr__(self):
        return str(self)

    def __str__(self):  # for printing purposes
        return "Label({0},{1},{2})".format(self.node, self.weight, self.res)

    def __lt__(self, other):
        # Less than operator for two Label objects
        return self.weight < other.weight and self.res <= other.res

    def __le__(self, other):
        # Less than or equal to operator for two Label objects
        return self.weight <= other.weight and self.res <= other.res

    def __eq__(self, other):
        # Equality operator for two Label objects
        return (self.weight == other.weight and self.res == other.res and
                self.node == other.node)

    def __hash__(self):
        # Redefinition of hash to avoid TypeError due to the __eq__ definition
        return id(self)

    def dominates(self, other, direction):
        # Return whether self dominates other.
        if direction == 'forward':
            return self < other
        else:
            return self.weight < other.weight and self.res >= other.res

    def getNewLabel(self, direction, weight, node, res):
        path = list(self.path)
        path.append(node)
        if direction == 'forward':
            res_new = list(map(add, self.res, res))
        else:
            res_new = list(map(sub, self.res, res))
        return Label(weight + self.weight, node, res_new, path)
