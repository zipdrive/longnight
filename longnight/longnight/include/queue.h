#pragma once


// A priority queue that doesn't care about access to anything but the topmost element. Uses a pairing heap structure.
template <class T>
class PriorityQueue
{
private:
	// A node in the priority queue.
	struct Node
	{
		// The value of the node.
		T value;

		// The priority of the node.
		int priority;

		// The child nodes.
		std::vector<Node*> children;

		/// <summary>Constructs a new node.</summary>
		/// <param name="value">The value of the node.</param>
		/// <param name="priority">The priority of the node. Low numbers are higher up in the queue.</param>
		Node(T value, int priority) : value(value), priority(priority) {}
	};

	/// <summary>Melds two nodes in place.</summary>
	/// <param name="n1">One node to meld.</param>
	/// <param name="n2">The other node to meld.</param>
	/// <returns>Either n1 and n2, whichever is the parent after the two have been melded.</returns>
	Node* meld(Node* n1, Node* n2)
	{
		// If either node is nullptr, return the other one.
		if (!n1) return n2;
		if (!n2) return n1;

		if (n1->priority < n2->priority)
		{
			n1->children.push_back(n2);
			return n1;
		}
		else
		{
			n2->children.push_back(n1);
			return n2;
		}
	}

	/// <summary>Merges a list of nodes into a single node by melding pairs of nodes.</summary>
	/// <param name="pairs">The list of nodes.</param>
	/// <returns>The single node that is the parent of the merged nodes.</returns>
	Node* merge_pairs(std::vector<Node*>& pairs)
	{
		if (pairs.size() == 0) return nullptr;
		if (pairs.size() == 1) return pairs[0];

		Node* n1 = pairs.back();
		pairs.pop_back();
		Node* n2 = pairs.back();
		pairs.pop_back();

		return meld(meld(n1, n2), merge_pairs(pairs));
	}

	// The node at the top of the heap.
	Node* parent = nullptr;

public:
	/// <summary>Checks whether the queue is empty.</summary>
	/// <returns>True if the queue has nothing in it, false otherwise.</returns>
	bool empty()
	{
		return parent == nullptr;
	}

	/// <summary>Inserts a value into the priority queue.</summary>
	/// <param name="value">The value to insert.</param>
	/// <param name="priority">The priority of the value. Low numbers are higher up in the queue.</param>
	void push(T value, int priority)
	{
		parent = meld(parent, new Node(value, priority));
	}

	/// <summary>Pops the value with the lowest priority number from the queue.</summary>
	/// <returns>The value in the queue with the lowest priority number.</returns>
	T pop()
	{
		if (!parent) throw std::string("Priority queue is empty.");

		T value = parent->value;

		Node* new_parent = merge_pairs(parent->children);
		delete parent;
		parent = new_parent;

		return value;
	}

	/// <summary>Peeks at the top of the queue.</summary>
	/// <param name="priority">A reference to be filled with the priority of the value at the top of the queue.</param>
	/// <param name="value">A reference to be filled with the value at the top of the queue.</param>
	/// <returns>True if the queue is nonempty, false otherwise.</returns>
	bool peek(int& priority, T& value)
	{
		if (parent)
		{
			priority = parent->priority;
			value = parent->value;
			return true;
		}

		return false;
	}

	/// <summary>Empties the queue into a vector.</summary>
	/// <param name="queue">Where all values in the queue are dumped in order.</param>
	void dump(std::vector<T>& queue)
	{
		queue.clear();
		while (!empty())
		{
			queue.push_back(pop());
		}
	}
};
