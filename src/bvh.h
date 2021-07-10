
struct AABB {
	V3 min;
	V3 max;
};

static AABB
mergeAABB(AABB* a1, AABB* a2) {
	AABB result;
	if(a1 && a2) {
		result.min = {
			a1->min.x < a2->min.x ? a1->min.x : a2->min.x,
			a1->min.y < a2->min.y ? a1->min.y : a2->min.y,
			a1->min.z < a2->min.z ? a1->min.z : a2->min.z
		};
		result.max = {
			a1->max.x > a2->max.x ? a1->max.x : a2->max.x,
			a1->max.y > a2->max.y ? a1->max.y : a2->max.y,
			a1->max.z > a2->max.z ? a1->max.z : a2->max.z
		};
		return(result);
	}
	result.min = a1->min;
	result.max = a1->max;
	return(result);
}

struct SphereAABB {
	AABB box;
	Sphere* s;
};

static SphereAABB
createSphereAABB(Sphere* s) {
	SphereAABB result = {};
	V3 radius = v3(s->r, s->r, s->r);
	result.box.min = s->pos - radius; 
	result.box.max = s->pos + radius;
	result.s = s;
	return(result);
}

enum BVHNodeType {
	Node = 1,
	Leaf,
};

struct BVHLeaf {
	AABB box;
	u32 childCount;
	SphereAABB* s1;
	SphereAABB* s2;
};

static BVHLeaf
constructBVHLeaf(SphereAABB* s1, SphereAABB* s2) {
	BVHLeaf result;
	if(s1 && s2) {
		result.childCount = 2;
		result.box = mergeAABB(&s1->box, &s2->box);
		result.s1 = s1;
		result.s2 = s2;
		return(result);
	}
	result.childCount = 1;
	result.box.min = s1->box.min;
	result.box.max = s1->box.max;
	result.s1 = s1;
	return(result);	
}

struct BVHNode {
	AABB box;
	BVHNodeType child1Type;
	BVHNodeType child2Type;
	union {
		BVHNode* child1;
		BVHLeaf* leaf1;
	};
	union {
		BVHNode* child2;
		BVHLeaf* leaf2;
	};
};

static BVHNode
constructBVHNodeFromLeafs(BVHLeaf* l1, BVHLeaf* l2) {
	BVHNode result = {};
	
	result.child1Type = Leaf;
	result.leaf1 = l1;
	if(l2) {
		result.box = mergeAABB(&l1->box, &l2->box);
		result.child2Type = Leaf;
		result.leaf2 = l2;
	}
	result.box = l1->box;
	return(result);
}

static BVHNode
constructBVHNodeFromNodes(BVHNode* n1, BVHNode* n2) {
	BVHNode result = {};
	
	result.child1Type = Node;
	result.child1 = n1;
	if(n2) {
		result.box = mergeAABB(&n1->box, &n2->box);
		result.child2Type = Node;
		result.child2 = n2;
		return(result);
	}
	result.box = n1->box;
	return(result);
}

static BVHNode
constructBVHNodeFromLeafNode(BVHNode* n, BVHLeaf* l) {
	BVHNode result = {};
	if(n && l) {
		result.box = mergeAABB(&n->box, &l->box);
		result.child1Type = Node;
		result.child1 = n;
		result.child2Type = Leaf;
		result.leaf2 = l;
		return(result);
	} else if(n) {
		result.box = n->box;
		result.child1Type = Node;
		result.child1 = n;
		return(result);
	}
	result.box = l->box;
	result.child2Type = Leaf;
	result.leaf2 = l;
	return(result);
}
