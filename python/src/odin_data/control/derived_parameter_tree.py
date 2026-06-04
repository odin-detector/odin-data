"""
Created on 3rd June 2026

:author: Famous Alele
"""
from functools import reduce
from odin.adapters.parameter_tree import ParameterAccessor
from odin.adapters.base_parameter_tree import (
    BaseParameterAccessor, BaseParameterTree, ParameterTreeError
)
class DerivedParameterAccessor(BaseParameterAccessor):
    # Valid metadata arguments that can be passed to ParameterAccess __init__ method.
    AUTO_METADATA_FIELDS = ("type", "access_mode")
    def __init__(self, path, getter=None, setter=None, **kwargs):
        # Initialise the superclass with the specified arguments
        super(DerivedParameterAccessor, self).__init__(path, getter, setter, **kwargs)
    
    def update_metadata(self, **kwargs):
        # Check metadata keyword arguments are valid
        for arg in kwargs:
            if arg not in (BaseParameterAccessor.VALID_METADATA_ARGS or DerivedParameterAccessor.AUTO_METADATA_FIELDS):
                raise ParameterTreeError("Invalid metadata argument: {}".format(arg))
        
        
        # Update metadata keywords from arguments
        super(DerivedParameterAccessor, self).metadata.update(kwargs)

        access_m = True
        if(kwargs["access_mode"] == "r"):
            access_m = False
        # Set the writeable metadata field if the setter is callable
        super(DerivedParameterAccessor, self).metadata["access_mode"] = access_m

class DerivedParameterTree(BaseParameterTree):
    def __init__(self, tree, metadata=None, mutable=False):
        """Initialise the ParameterTree object.

        This constructor recursively initialises the ParameterTree object based on the specified
        arguments. The tree initialisation syntax follows that of the BaseParameterTree
        implementation.

        :param tree: dict representing the parameter tree
        :param mutable: Flag, setting the tree
        """
        # Set the accessor class used by this tree to ParameterAccessor
        self.accessor_cls = ParameterAccessor
        # Flag, if set to true, allows nodes to be replaced and new nodes created
        self.mutable = mutable

        # list of paths to mutable parts. Not sure this is best solution
        self.mutable_paths = []

        # Recursively check and inisynchronoustialise the tree
        self._tree = self._build_tree(tree, metadata)
        
        # Initialise the superclass with the speccified parameters
        super(DerivedParameterTree, self).__init__(None, None)

        # swap the parameter tree we just constructed with what the base class has
        self._tree, super(DerivedParameterTree, self)._tree = super(DerivedParameterTree, self)._tree, self._tree

    def _get_nested_value(data, path):
        return reduce(lambda d, key: d[key], path, data)
    
    def _build_tree(self, node, metadata, path=''):
        """Recursively build and expand out a tree or node.

        This internal method is used to recursively build and expand a tree or node,
        replacing elements as found with appropriate types, e.g. ParameterAccessor for
        a set/get pair, the internal tree of a nested ParameterTree.

        :param node: node to recursively build
        :param path: path to node within overall tree
        :returns: built node
        """
        # If the node is a parameter tree instance, replace with its own built tree
        if isinstance(node, type(self)):
            if node.mutable:
                super(DerivedParameterTree, self).mutable_paths.append(path)
            return node.tree  # this breaks the mutability of the sub-tree. hmm

        # Convert node tuple into the corresponding ParameterAccessor, depending on type of
        # fields
        if isinstance(node, tuple):
            if len(node) == 1:
                # Node is (value)
                if(metadata is not None):
                    param_metadata = self._get_nested_value(metadata, path.split('/'))
                    param = self.accessor_cls(path, node[0], **param_metadata)
                else:
                    param = self.accessor_cls(path, node[0])

            elif len(node) == 2:
                if isinstance(node[1], dict):
                    # Node is (value, {metadata})
                    param = self.accessor_cls(path, node[0], **node[1])
                    
                else:
                    # Node is (getter, setter)
                    param = self.accessor_cls(path, node[0], node[1])

            elif len(node) == 3 and isinstance(node[2], dict):
                # Node is (getter, setter, {metadata})
                param = self.accessor_cls(path, node[0], node[1], **node[2])

            else:
                raise ParameterTreeError("{} is not a valid leaf node".format(repr(node)))

            return param

        # Convert list or non-callable tuple to enumerated dict
        if isinstance(node, list):
            return [self._build_tree(elem, path=path) for elem in node]

        # Recursively check child elements
        if isinstance(node, dict):
            return {k: self._build_tree(
                v, metadata=metadata, path=path + str(k) + '/') for k, v in node.items()}

        return node
    
    def replace(self, path, data, metadata=None):
        """Replaces a branch of parameters in a tree.

        This method sets the values of parameters in a tree, based on the data passed to it
        as a nested dictionary of parameter and value pairs. Any structure below the insertion
        point in the exising tree is replaced with this new structure.

        :param path: path to set parameters for in the tree
        :param data: nested dictionary representing structure to replace at the path
        """
        self.set(path, data, metadata, replace=True)

    def set(self, path, data, metadata, replace=False):
        """Set the values of the parameters in a tree.

        This method sets the values of parameters in a tree, based on the data passed to it
        as a nested dictionary of parameter and value pairs. The updated parameters are merged
        into the existing tree recursively.

        :param path: path to set parameters for in the tree
        :param data: nested dictionary representing values to update at the path
        :param replace: if set to true then the structure is replaced rather than merged
        """
        # Expand out any lists/tuples
        data = self._build_tree(data, metadata)

        # Get subtree from the node the path points to
        levels = path.split('/')
        if levels[-1] == '':
            del levels[-1]

        merge_parent = None
        merge_child = super(DerivedParameterTree, self)._tree

        # Descend the tree and validate each element of the path
        for level in levels:
            try:
                merge_parent = merge_child
                if isinstance(merge_child, dict):
                    merge_child = merge_child[level]
                else:
                    merge_child = merge_child[int(level)]
            except (KeyError, ValueError, IndexError):
                raise ParameterTreeError("Invalid path: {}".format(path))

        # Add trailing / to paths where necessary
        if path and path[-1] != '/':
            path += '/'

        # Merge data with tree
        if replace:
            if not super(DerivedParameterTree, self).mutable:
                raise ParameterTreeError("Invalid replace attempt: tree not mutable")
            merged = data
        else:
            merged = self._merge_tree(merge_child, data, path)

        # Add merged part to tree, either at the top of the tree or at the
        # appropriate level speicfied by the path
        if not levels:
            super(DerivedParameterTree, self)._tree = merged
            return
        if isinstance(merge_parent, dict):
            merge_parent[levels[-1]] = merged
        else:
            merge_parent[int(levels[-1])] = merged

    
