/* 
* @Author: BlahGeek
* @Date:   2015-01-14
* @Last Modified by:   BlahGeek
* @Last Modified time: 2015-12-25
*/

#include <iostream>
#include "./kdtree.h"
#include <algorithm>

using namespace hp;

std::pair<cl_float, cl_float> KDTree::Node::triangleMinMax(cl_int4 geo, int dimension) {
    cl_float val[3];
    for(int i = 0 ; i < 3 ; i += 1)
        val[i] = points[geo.s[i]].s[dimension];
    cl_float min = *std::min_element(val, val+3);
    cl_float max = *std::max_element(val, val+3);
    return std::make_pair(min, max);
}

void KDTree::Node::calcMinMaxVals() {
    for(int d = 0 ; d < 3 ; d += 1) {
        for(auto & geo_index: geo_indexes) {
            auto geo = geometries[geo_index];
            auto result = this->triangleMinMax(geo, d);
            min_vals[d].push_back(result.first);
            max_vals[d].push_back(result.second);
        }
        std::sort(min_vals[d].begin(), min_vals[d].end());
        std::sort(max_vals[d].begin(), max_vals[d].end());
    }
}

void KDTree::Node::setBoxSize() {
    for(int d = 0 ; d < 3 ; d += 1) {
        box_start.s[d] = min_vals[d].front() - 1e-3f;
        box_end.s[d] = max_vals[d].back() + 1e-3f;
    }
}

#define TRAVERSAL_COST 1.f
#define TRIANGLE_INTERSECT_COST 3.f


std::pair<cl_float, cl_float> KDTree::Node::findBestSplit(int dimension) {
    std::vector<cl_float> all_vals(geo_indexes.size() * 2);
    memcpy(all_vals.data(), min_vals[dimension].data(), geo_indexes.size() * sizeof(cl_float));
    memcpy(all_vals.data() + geo_indexes.size(), 
           max_vals[dimension].data(), 
           geo_indexes.size() * sizeof(cl_float));
    std::sort(all_vals.begin(), all_vals.end());

    cl_float best_cost = CL_FLT_MAX;
    cl_float best_pos = 0;

    auto min_it = min_vals[dimension].begin();
    auto max_it = max_vals[dimension].begin();

    int leftT = 0; int rightT = geo_indexes.size();
    for(auto & val: all_vals) {
        if(val <= box_start.s[dimension] || val >= box_end.s[dimension])
            continue;
        while(min_it != min_vals[dimension].end() && 
              *min_it <= val) {
            // hp_log("left shift %f", *min_it);
            min_it ++;
            leftT ++;
        }
        while(max_it != max_vals[dimension].end() &&
              *max_it < val) {
            // hp_log("right shift %f", *max_it);
            max_it ++;
            rightT --;
        }
        cl_float area_length = box_end.s[dimension] - box_start.s[dimension];
        cl_float left_length = val - box_start.s[dimension];
        // hp_log("find: %d; %d %d; %f", dimension, leftT, rightT, val);
        cl_float cost = TRAVERSAL_COST + TRIANGLE_INTERSECT_COST * 
                        (leftT * left_length / area_length + rightT * (1.f - left_length / area_length));
        if(cost < best_cost) {
            best_cost = cost;
            best_pos = val;
        }
    }

    return std::make_pair(best_pos, best_cost);
}

#define LEAF_GEOMETRIES 5

void KDTree::Node::split() {
    // hp_log("I'm node (%f %f %f)->(%f %f %f) with %lu triangles (this=%08x)",
    //        box_start.s[0], box_start.s[1], box_start.s[2],
    //        box_end.s[0], box_end.s[1], box_end.s[2],
    //        geo_indexes.size(), (void *)this);

    if(geo_indexes.size() < LEAF_GEOMETRIES) return;

    cl_float no_split_cost = TRIANGLE_INTERSECT_COST * this->geo_indexes.size();

    cl_float best_pos[3]; cl_float best_val[3];
    for(int d = 0 ; d < 3 ; d += 1) {
        auto result = this->findBestSplit(d);
        best_pos[d] = result.first;
        best_val[d] = result.second;
    }
    auto best_val_it = std::min_element(best_val, best_val + 3);
    int best_dimension = best_val_it - best_val;

    if(*best_val_it > no_split_cost) {
        // hp_log("Split cost too much, do not split");
        return;
    }
    
    // hp_log("Going to split %dth dimension at position %f",
    //        best_dimension, best_pos[best_dimension]);

    // hp_assert(best_pos[best_dimension] >= box_start.s[best_dimension]);
    // hp_assert(best_pos[best_dimension] <= box_end.s[best_dimension]);

    if(best_pos[best_dimension] <= box_start.s[best_dimension] ||
       best_pos[best_dimension] >= box_end.s[best_dimension]) {
        // hp_log("Best-split-position is out of border, do not split");
        return;
    }

    this->left = std::make_unique<KDTree::Node>(points, geometries);
    this->right = std::make_unique<KDTree::Node>(points, geometries);
    this->left->parent = this->right->parent = this;
    this->left->box_start = this->right->box_start = this->box_start;
    this->left->box_end = this->right->box_end = this->box_end;

    this->left->box_end.s[best_dimension] = best_pos[best_dimension];
    this->right->box_start.s[best_dimension] = best_pos[best_dimension];

    // hp_log("Left child: (%f %f %f)->(%f %f %f)",
    //        this->left->box_start.s[0], this->left->box_start.s[1], this->left->box_start.s[2],
    //        this->left->box_end.s[0], this->left->box_end.s[1], this->left->box_end.s[2]);
    // hp_log("Right child: (%f %f %f)->(%f %f %f)",
    //        this->right->box_start.s[0], this->right->box_start.s[1], this->right->box_start.s[2],
    //        this->right->box_end.s[0], this->right->box_end.s[1], this->right->box_end.s[2]);

    for(auto & geo_index: geo_indexes) {
        auto geo = geometries[geo_index];
        std::vector<cl_float3> this_points = {points[geo.s[0]], points[geo.s[1]], points[geo.s[2]]};
        if(!std::all_of(this_points.begin(), this_points.end(), [&](const cl_float3 & p) ->bool {
            return p.s[best_dimension] > best_pos[best_dimension];
        })) this->left->geo_indexes.push_back(geo_index);
        if(!std::all_of(this_points.begin(), this_points.end(), [&](const cl_float3 & p) ->bool {
            return p.s[best_dimension] < best_pos[best_dimension];
        })) this->right->geo_indexes.push_back(geo_index);
        // if(this->left->contain(geo, best_dimension)) this->left->geo_indexes.push_back(geo_index);
        // if(this->right->contain(geo, best_dimension)) this->right->geo_indexes.push_back(geo_index);
    }
    this->geo_indexes.clear();

    // hp_log("After split: left have %lu triangles, right have %lu triangles",
    //        this->left->geo_indexes.size(), this->right->geo_indexes.size());

    this->left->calcMinMaxVals();
    // this->left->setBoxSize();
    this->left->split();

    this->right->calcMinMaxVals();
    // this->right->setBoxSize();
    this->right->split();
}

void KDTree::Node::removeEmptyNode() {
    if(left == nullptr && right == nullptr) return;
    left->removeEmptyNode();
    right->removeEmptyNode();

    if(left && left->geo_indexes.size() == 0 && left->left == nullptr && left->right == nullptr) {
        this->box_start = right->box_start;
        this->box_end = right->box_end;
        this->geo_indexes = right->geo_indexes;
        this->left = std::move(right->left);
        hp_assert(right->left == nullptr);
        std::unique_ptr<KDTree::Node> tmp = std::move(right->right);
        hp_assert(right->right == nullptr);
        this->right = std::move(tmp);
        if(this->right)
            this->right->parent = this;
        if(this->left)
            this->left->parent = this;
        return;
    }
    if(right && right->geo_indexes.size() == 0 && right->left == nullptr && right->right == nullptr) {
        this->box_start = left->box_start;
        this->box_end = left->box_end;
        this->geo_indexes = left->geo_indexes;
        this->right = std::move(left->right);
        hp_assert(left->right == nullptr);
        std::unique_ptr<KDTree::Node> tmp = std::move(left->left);
        hp_assert(left->left == nullptr);
        this->left = std::move(tmp);
        if(this->right)
            this->right->parent = this;
        if(this->left)
            this->left->parent = this;
        return;
    }
}

int KDTree::Node::debugPrint(int depth, int id) {
    for(int i = 0 ; i < depth ; i += 1)
        fprintf(stderr, "  ");
    fprintf(stderr, "ID %d", id++);
    fprintf(stderr, "(%f %f %f)->(%f %f %f) ",
           box_start.s[0], box_start.s[1], box_start.s[2],
           box_end.s[0], box_end.s[1], box_end.s[2]);
    if(geo_indexes.size() != 0) {
        fprintf(stderr, " LEAF, size = %lu\n", geo_indexes.size());
        for(auto & geo_id: geo_indexes) {
            fprintf(stderr, "...");
            for(int i = 0 ; i < depth ; i += 1)
                fprintf(stderr, "  ");
            fprintf(stderr, "Triangle %d %d %d\n", geometries[geo_id].s[0],
                    geometries[geo_id].s[1], geometries[geo_id].s[2]);
        }
    }
    else {
        fprintf(stderr, "\n");
        if(left)
            id = left->debugPrint(depth + 1, id);
        if(right)
            id = right->debugPrint(depth + 1, id);
    }
    return id;
}

KDTree::KDTree(std::string filename, std::string mtl_basepath): 
Scene(filename, mtl_basepath) {
    this->root = std::make_unique<KDTree::Node>(this->points, this->geometries);
    for(size_t i = 0 ; i < this->geometries.size() ; i += 1)
        this->root->geo_indexes.push_back(i);
    this->root->calcMinMaxVals();
    this->root->setBoxSize();
    this->root->split();
    this->root->removeEmptyNode();

    // this->root->debugPrint();
}

std::pair<std::vector<KDTreeNodeHeader>, std::vector<cl_int>>
    KDTree::getData() {

        std::vector<KDTree::Node *> nodes;
        std::map<KDTree::Node *, int> nodes_map;
        std::function<void(KDTree::Node * node)> walk;
        walk = [&](KDTree::Node * node) {
            nodes_map[node] = nodes.size();
            nodes.push_back(node);
            if(node->left) walk(node->left.get());
            if(node->right) walk(node->right.get());
        };
        walk(this->root.get());

        std::vector<KDTreeNodeHeader> header_data;
        std::vector<cl_int> triangle_data;

        for(size_t i = 0 ; i < nodes.size() ; i += 1) {
            auto node = nodes[i];
            KDTreeNodeHeader header;
            header.data = -1;
            if(node->geo_indexes.size() > 0) {
                header.data = triangle_data.size();
                triangle_data.push_back(node->geo_indexes.size());
                for(auto & x: node->geo_indexes)
                    triangle_data.push_back(x);
            }
            header.box_start = node->box_start;
            header.box_end = node->box_end;
            header.child = (node->left == nullptr) ? -1 : 
                            nodes_map[node->left.get()];
            header.parent = (node->parent == nullptr) ? -1 :
                            nodes_map[node->parent];
            header.sibling = -1;
            if(node->parent != nullptr && node->parent->right != nullptr &&
               node->parent->right.get() != node) {
                header.sibling = nodes_map[node->parent->right.get()];
            }

            header_data.push_back(header);
        }

        return std::make_pair(header_data, triangle_data);

    }
