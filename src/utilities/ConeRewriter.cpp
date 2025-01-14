#include <set>
#include <queue>
#include <vector>
#include  <random>
#include <utility>
#include "ast_opt/utilities/ConeRewriter.h"
#include "ast_opt/ast/OperatorExpression.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/visitor/GetAllNodesVisitor.h"

std::unique_ptr<AbstractNode> ConeRewriter::rewriteAst(std::unique_ptr<AbstractNode> &&ast_in) {
  /// map of all mult depths, will be filled as we go along during computation
  MultDepthMap multDepths;
  MultDepthMap revMultDepths;

  /// MinDepth for finding cones
  auto minDepth = computeMinDepth(ast_in.get(), ast_in.get(), multDepths, revMultDepths);

  /// set of all reducible cones
  auto delta = getReducibleCone(ast_in.get(), ast_in.get(), minDepth, multDepths, revMultDepths);

  /// C^{AND} circuit consisting of critical AND-nodes that are connected if there is a multiplicative depth-2
  /// path in between two of those nodes in the initial circuit
  /// each cone δ ∈ Δ has an unique terminal AND node in C^{AND}
  //std::vector<AbstractNode *> cAndCkt = getAndCriticalCircuit(*ast_in, delta);

  /// minimum set of reducible cones
  // std::vector<AbstractNode *> deltaMin = selectCones(*ast_in, cAndCkt);

  /// Perform actual rewriting
  return rewriteCones(std::move(ast_in), delta);
}

std::vector<AbstractNode *> ConeRewriter::getReducibleCone(AbstractNode *root,
                                                           AbstractNode *v,
                                                           int minDepth,
                                                           MultDepthMap multiplicativeDepths,
                                                           MultDepthMap revMultDepths) {

  // return empty set if minDepth is reached
  if (computeMultDepthL(v, multiplicativeDepths)==minDepth) {
    return std::vector<AbstractNode *>();
  }

  // get predecessor (children) { p ∈ pred(v) | l(p) = l(v) - d(v) }: put them in the vector called pvec (paper: P)
  auto pvec = std::vector<AbstractNode *>();
  for (auto &p: *v) {
    if (computeMultDepthL(&p, multiplicativeDepths)==computeMultDepthL(v, multiplicativeDepths) - depthValue(v)) {
      pvec.push_back(&p);
    }
  }
  // return v if at least one predecessor of v is non-critical (i.e |pvec| < 2) and v is an AND-gate
  if (pvec.size() < 2 && dynamic_cast<BinaryExpression *>(v)->getOperator().toString()=="&&") {
    // return set consisting of start node v only
    return std::vector<AbstractNode *>{v};
  }

  // determine reducible input cones
  std::vector<std::vector<AbstractNode *>> deltaR;
  std::vector<AbstractNode *> delta;
  for (auto &p : pvec) {
    std::vector<AbstractNode *> intermedResult =
        getReducibleCone(root,
                         p,
                         computeMinDepth(p, root, multiplicativeDepths, revMultDepths),
                         multiplicativeDepths,
                         revMultDepths);
    if (!intermedResult.empty()) deltaR.push_back(intermedResult);
  }

  // return empty set if either one of the following is true:
  // a. v is not a LogicalExpr
  // b. v is a LogicalExpr but not an AND- or XOR-gate
  // b. v is an AND-gate and deltaR is empty
  // c. v is a XOR-gate and size of deltaR does not equal size of pvec
  if (v==nullptr ||
      !(dynamic_cast<BinaryExpression *>(v)->getOperator().toString()=="&&"
          || dynamic_cast<BinaryExpression *>(v)->getOperator().toString()=="||") ||
      (dynamic_cast<BinaryExpression *>(v)->getOperator().toString()=="&&" && deltaR.empty()) ||
      (dynamic_cast<BinaryExpression *>(v)->getOperator().toString()=="||" && deltaR.size()!=pvec.size())) {
    return std::vector<AbstractNode *>();
  }

  if (dynamic_cast<BinaryExpression *>(v)->getOperator().toString()=="&&") {
    // both cones must be reducible because deltaR is non-empty -> pick a random one, and assign to delta
    delta = deltaR[rand()%deltaR.size()];
  } else if (dynamic_cast<BinaryExpression *>(v)->getOperator().toString()=="||") {
    // critical cones must be reducible because size of deltaR equals size of P
    // flatten vector deltaR consisting of sets generated each by getReducibleCones
    std::vector<AbstractNode *> flattenedDeltaR;
    flattenVectors(flattenedDeltaR, deltaR);
    // add all elements of flattened deltaR to delta
    addElements(delta, flattenedDeltaR);
  }
  // return 𝛅 ⋃ {v}
  delta.push_back(v);
  return delta;
}

// TODO: reverse?
std::vector<AbstractNode *> ConeRewriter::getAndCriticalCircuit(AbstractNode &root, std::vector<AbstractNode *> delta) {
  // remove non-AND nodes from delta (note: delta is passed as copy-by-value) as delta may also include XOR nodes
  delta.erase(remove_if(delta.begin(), delta.end(), [](AbstractNode *d) {
    auto lexp = dynamic_cast<BinaryExpression *>(d);
    return (lexp==nullptr || lexp->getOperator().toString()!="&&");
  }), delta.end());

  //std::cout << "size" << delta[0]->toString(false) << std::endl;
  // duplicate critical nodes to create new circuit C_{AND} as we do not want to modify the original circuit
  std::unordered_map<std::string, AbstractNode *> cAndMap;
  std::vector<AbstractNode *> cAndResultCkt;
  for (auto &v : delta) {
    // clone node and remove parents (argument of clone: nullptr)
    std::cout << "Cloning node: " << v->toString(false) << std::endl;
    auto clonedNode = v->clone(nullptr);

    // a back-link to the node in the original circuit

    std::cout << "Inserting: " << v->getUniqueNodeId() << " into underlying_nodes" << std::endl;
    underlying_nodes.insert(std::make_pair<std::string, AbstractNode *>(v->getUniqueNodeId(), &*v));
    cAndMap.emplace(v->getUniqueNodeId(), &*clonedNode);
    cAndResultCkt.emplace_back(&*clonedNode);
    std::cout << "result1 size: " << cAndResultCkt.size() << std::endl;
  }

  // in case that there are less than two nodes, we can not connect any two nodes
  if (delta.size() < 2) {
    std::cout << "Ret" << std::endl;
    return cAndResultCkt;
  }
  // TODO: understand
  // check if there are depth-2 critical paths in between critical nodes in the original ckt
  for (auto &v : delta) {
    std::queue<AbstractNode *> q{{v}};
    while (!q.empty()) {
      auto curNode = q.front();
      q.pop();
      // for children
      for (auto &parent : curNode->getParent()) {
        auto parentLexp = dynamic_cast<BinaryExpression *>(&parent);
        // if the parent is a LogicalExpr of type AND-gate
        if (parentLexp!=nullptr && parentLexp->getOperator().toString()=="&&") {
          // check if this parent is a critical node, if yes: connect both nodes
          if (std::find(delta.begin(), delta.end(), parentLexp)!=delta.end()) {
            AbstractNode *copiedV = cAndMap[v->getUniqueNodeId()];
            AbstractNode *copiedParent = cAndMap[parent.getUniqueNodeId()];
            copiedV->addParent(copiedParent);
            copiedParent->addChild(copiedV);
          }
        } else {  // continue if child is not a LogicalExpr --> node does not influence the mult. depth
          q.push(&parent);
        }
      }
    }
  }
  return cAndResultCkt;
}

std::vector<AbstractNode *> ConeRewriter::selectCones(AbstractNode &root, std::vector<AbstractNode *> cAndCkt) {

  std::cout << "Selecting cones" << std::endl;

  std::vector<AbstractNode *> deltaMin;

  std::cout << cAndCkt.size() << std::endl;

  while (!cAndCkt.empty()) {
    std::cout << "Computing node flows" << std::endl;
    // compute all node flows f^{+}(v) in cAndCkt
    std::map<AbstractNode *, float> nodeFlows = compFlow(cAndCkt);


    // reverse circuit edges and compute all ascending node flows f^{-}(v)
    reverseEdges(cAndCkt);
    std::map<AbstractNode *, float> nodeFlowsAscending = compFlow(cAndCkt);

    // compute f(v) for all nodes
    std::map<AbstractNode *, float> flows;
    for (auto &n : cAndCkt) flows[n] = nodeFlowsAscending[n]*nodeFlows[n];

    // find the node with the largest flow u
    AbstractNode *u = std::max_element(
        flows.begin(),
        flows.end(),
        [](const std::pair<AbstractNode *, float> &p1, const std::pair<AbstractNode *, float> &p2) {
          return p1.second < p2.second;
        })->first;

    // remove node u
    // - remove any edges pointing to node u as child
    for (auto &p : u->getParentList()) p->removeChild(u);

    // - remove any edges pointing to node u as parent
    for (auto &p : u->getChildrenList()) p->removeParent(u);

    // - remove node u from cAndCkt
    cAndCkt.erase(std::remove(cAndCkt.begin(), cAndCkt.end(), u), cAndCkt.end());

    // add critical cone ending at node u to deltaMin
    deltaMin.push_back(u);
  }

  return deltaMin;
}

std::unique_ptr<AbstractNode> ConeRewriter::rewriteCones(std::unique_ptr<AbstractNode> &&ast,
                                                         std::vector<AbstractNode *> &coneEndNodes) {
////  // Assumption: δ ∈ coneEndNodes represents a cone that ends at node δ
//  for (auto coneEnd : coneEndNodes) {
////    // we need to get the node he underlying circuit as C^{AND} only contains a limited subset of nodes
//    coneEnd = underlying_nodes.find(coneEnd->getUniqueNodeId())->second;
////    // determine bounds of the cone
////    // -- upper bound: parent node of cone end
//    auto& rNode = coneEnd->getParent(); // this is node r in the paper [Aubry et al. Figure 2]
////
////    // U_y, the multi-input XOR, is represented as multiple XOR nodes in our tree:
////    // [v_1, ..., v_n] --> xorN --> xorN-1 ---> xorN-2 ---> ... ---> xor1 --> sNode v_t.
////    // We denote xorN as xorEndNode and xor1 as xorStartNode. We know that xorStartNode must be the first node in the
////    // cone, i.e., the first child of the cone's end node.
//    auto coneEndAsLogicalExpr = dynamic_cast<BinaryExpression *>(coneEnd);
//    auto[xorStartNode, a_t] = getCriticalAndNonCriticalInput(coneEndAsLogicalExpr);
////
////    // we "cut-off" parent edge between coneEnd and a_t in order to reconnect a_t later (but we keep a_t's children!)
//    std::unique_ptr<AbstractExpression> a_t_taken;
//    auto coneEndcast = dynamic_cast<BinaryExpression *>(coneEnd);
//    if( !isCriticalNode(&coneEndcast->getLeft())) {
//      a_t_taken = coneEndcast->takeLeft();
//    } else { a_t_taken = coneEndcast->takeRight(); }
////
////    // -- lower bound: first AND node while following critical path
////    // find the ancestors of δ that are LogicalExpr --> start nodes (v_1, ..., v_n) of cone
//    std::vector<AbstractNode *> coneStartNodes;
//    std::queue<AbstractNode *> q{{coneEnd}};
//    while (!q.empty()) {
//      auto curNode = q.front();
//      q.pop();
//      auto curNodeLexp = dynamic_cast<BinaryExpression *>(curNode);
////      // if curNode is not the cone end node and a logical-AND expression, we found one of the end nodes
////      // in that case we can stop exploring this path any further
//      if (curNode!=coneEnd && curNodeLexp!=nullptr && curNodeLexp->toString(false) == "LOGICAL_AND") {
//        coneStartNodes.push_back(curNode);
//      } else {  // otherwise we need to continue search by following the critical path
////        // add parent nodes of current nodes -> continue BFS traversal
//          for (auto &child : *curNode) {
//       //   for (auto &child : curNode->getParentsNonNull()) {
//          if (isCriticalNode(&child)) { q.push(&child); }
//        }
//      }
//    }
////
//    std::vector<AbstractNode *> finalXorInputs;
////    // It should not make a difference which of the cone start nodes we take - all of them should have the same parent.
//    auto& xorEndNode = coneStartNodes.front()->getParent();
////
//    for (auto &startNode : coneStartNodes) assert(startNode->getParent()==xorEndNode);
////
////    // check whether we need to handle non-critical inputs y_1, ..., y_m
//    if (dynamic_cast<OperatorExpression *>(&xorEndNode)->getOperator().toString() == "LOGICAL_AND" && &xorEndNode==coneEnd) {
////      // if there are no non-critical inputs y_1, ..., y_m then the cone's end and cone's start are both connected with
////      // each other, i.e y_1 is an AND node
////      // remove the edge between the start nodes and the end node
////
//      std::vector<std::unique_ptr<OperatorExpression>> v1_to_vn(coneEnd->countChildren());
//
////     // for (auto &node : coneStartNodes) {
////     // v1_to_vn.push_back(node->take());
//     for (int ii = 0; ii < coneEnd->countChildren(); ii++) {
//        auto child = dynamic_cast<OperatorExpression *>(&xorEndNode)->takeChild(ii);
//       // v1_to_vn.emplace_back(*child); // This confuses me...
//        // coneEnd->removeChild(node, true);
//        }
//    } else {
////      // collect all non-critical inputs y_1, ..., y_m in between xorStartNode up to xorEndNode
//      std::vector<AbstractNode *> inputsY1ToYm;
//      auto currentNode = xorStartNode;
//      while (true) {
//        auto *currentNodeAsLogicalExpr = dynamic_cast<BinaryExpression *>(currentNode);
//        if (currentNode==nullptr)
//          throw std::logic_error(
//              "AbstractNode between cone end and cone start node is expected to be a logical expression!");
//        auto[critIn, nonCritIn] = getCriticalAndNonCriticalInput(currentNodeAsLogicalExpr);
////TODO        currentNode->removeChild(nonCritIn, false);
////TODO        nonCritIn->removeParent(currentNode, false);
//        inputsY1ToYm.push_back(nonCritIn);
//        currentNode = critIn;
////        // termination criteria (1): we reached the end of the XOR chain
//        if (currentNode->getUniqueNodeId() == xorEndNode.getUniqueNodeId()) {
//          auto *xorEndNodeAsLogicalExpr = dynamic_cast<BinaryExpression *>(currentNode);
////          // if only one of both is critical, we need to enqueue the non-critical of them.
////          // exactly one of both inputs is non-critical
////          //   <=> left is critical XOR right is critical
////          //   <=> (left is critical) unequal (right is critical)
//          if (isCriticalNode(&xorEndNodeAsLogicalExpr->getLeft())
//              !=isCriticalNode(&xorEndNodeAsLogicalExpr->getRight())) {
////            // then we need to collect this non-critical input
//            auto nonCriticalInput = getCriticalAndNonCriticalInput(xorEndNodeAsLogicalExpr).second;
//            inputsY1ToYm.push_back(nonCriticalInput);
//          }
////          // stop traversing further as we reached the xorEndNode
//          break;
//        }
////        // termination criteria (2): we already reached a cone start node
//        if (std::find(coneStartNodes.begin(), coneStartNodes.end(), currentNode)!=coneStartNodes.end()) {
//          break;
//        }
//      }
////
////      // XorEndNode: remove incoming edges from nodes v_1, ..., v_n
////      for (auto &p : xorEndNode->getChildrenNonNull()) {
////        // keep the operator node -> only remove those nodes that are no operators
////        if (dynamic_cast<Operator *>(p)==nullptr) xorEndNode->removeChild(p, true);
////      }
////      // XorStartNode: remove incoming edge from xorStartNode to v_t
////      for (auto &c : xorStartNode->getParentsNonNull()) {
////        c->removeChild(xorStartNode, true);
////      }
////      // check that at least one y_i is present
////      if (inputsY1ToYm.empty())
////        throw std::logic_error("Unexpected number (0) of non-critical inputs y_1, ..., y_m.");
////
////      // build new node u_y that is connected to inputs y_1, ..., y_m and a_t
////      AbstractExpression *u_y_rightOperand = nullptr;
////      if (inputsY1ToYm.size()==1) {
////        // if y_1 only exists: connect input y_1 directly to u_y -> trivial case
////        u_y_rightOperand = dynamic_cast<AbstractExpression *>(inputsY1ToYm.front());
////        //u_y_rightOperand = inputsY1ToYm.front()->castTo<AbstractExpression>();
////      } else if (inputsY1ToYm.size() > 1) {  // otherwise there are inputs y_1, y_2, ..., y_m
////        // otherwise build XOR chain of inputs and connect last one as input of u_y
////        std::vector<AbstractNode *> yXorChain =
////            rewriteMultiInputGateToBinaryGatesChain(inputsY1ToYm, LOGICAL_XOR);
////        u_y_rightOperand = dynamic_cast<AbstractExpression *>(yXorChain.back());
////        //u_y_rightOperand = yXorChain.back()->castTo<AbstractExpression>();
////      }
////      auto *u_y = new BinaryExpression(a_t->castTo<AbstractExpression>(), LOGICAL_AND, u_y_rightOperand);
////      finalXorInputs.push_back(u_y);
//    } // end of handling non-critical inputs y_1, ..., y_m
////
////    // for each of these start nodes v_i
////    for (auto sNode : coneStartNodes) {
////      // determine critical input a_1^i and non-critical input a_2^i of v_i
////      auto *sNodeAsLogicalExpr = dynamic_cast<BinaryExpression *>(sNode);
////      if (sNodeAsLogicalExpr==nullptr)
////        throw std::logic_error("Start node of cone is expected to be of type LogicalExpr!");
////      auto[criticalInput, nonCriticalInput] = getCriticalAndNonCriticalInput(sNodeAsLogicalExpr);
////
////      // remove critical input of v_i
////      criticalInput->removeParent(sNode, false);
////     // sNode->removeChild(criticalInput, false);
////
////      // remove all outgoing edges of v_i
////    //  sNode->removeChild(criticalInput, true);
////
////      // add non-critical input of v_t (coneEnd) named a_t as input to v_i
////      auto originalOperator = sNodeAsLogicalExpr->getOperator();
////      sNodeAsLogicalExpr->setAttributes(dynamic_cast<BinaryExpression *>(nonCriticalInput),
////                                        originalOperator,
////                                        dynamic_cast<AbstractExpression *>(a_t));
////
////      // create new logical-AND node u_i and set v_i as input of u_i
////      auto leftOp = dynamic_cast<AbstractExpression>(criticalInput);
////      //auto leftOp = criticalInput->castTo<AbstractExpression>();
////      auto uNode = new BinaryExpression(leftOp, LOGICAL_AND, sNodeAsLogicalExpr);
////      finalXorInputs.push_back(uNode);
////    }
////    // convert multi-input XOR into binary XOR nodes
////    std::vector<AbstractNode *> xorFinalGate =
////        rewriteMultiInputGateToBinaryGatesChain(finalXorInputs, LOGICAL_XOR);
////    for (auto &gate : xorFinalGate) { gate->getUniqueNodeId(); }
////
////    // remove coneEnd
////    astToRewrite.deleteNode(&coneEnd, true);
////    assert(coneEnd==nullptr);
////
////    // connect final XOR (i.e., last LogicalExpr in the chain of XOR nodes) to cone output
////    rNode->addChild(xorFinalGate.back(), false);
////    xorFinalGate.back()->addParent(rNode, false);    // convert multi-input XOR into binary XOR nodes
////    std::vector<AbstractNode *> xorFinalGate =
////        rewriteMultiInputGateToBinaryGatesChain(finalXorInputs, LogCompOp::LOGICAL_XOR);
////    for (auto &gate : xorFinalGate) { gate->getUniqueNodeId(); }
////
////    // remove coneEnd
////    astToRewrite.deleteNode(&coneEnd, true);
////    assert(coneEnd==nullptr);
////
////    // connect final XOR (i.e., last LogicalExpr in the chain of XOR nodes) to cone output
////    rNode->addChild(xorFinalGate.back(), false);
////    xorFinalGate.back()->addParent(rNode, false);
////
//  } //end: for (auto coneEnd : coneEndNodes)
  return std::move(ast);
}

int ConeRewriter::computeMinDepth(AbstractNode *v,
                                  AbstractNode *ast,
                                  MultDepthMap multDepthMap,
                                  MultDepthMap reversedMultDepthsMap) {
  // find a non-critical input (child) node p of v
  auto isNoOperatorNode = [](AbstractNode *n) { return (dynamic_cast<Operator *>(n)==nullptr); };
  for (auto &p : *v) {
    if (!isCriticalNode(&p, ast, multDepthMap, reversedMultDepthsMap) && isNoOperatorNode(&p)) {
      // std::cout << "Noncrit input " << p.toString(false) << std::endl;
      return computeMultDepthL(&p, multDepthMap) + 1;
    }
  }
  // error (-1) if no non-critical child node
  return -1;
}

bool ConeRewriter::isCriticalNode(AbstractNode *n,
                                  AbstractNode *ast,
                                  MultDepthMap multDepthmap,
                                  MultDepthMap reversedMultDepthsMap) {
  int l = computeMultDepthL(n, multDepthmap);
  int r = computeReversedMultDepthR(n, reversedMultDepthsMap);
  return (getMaximumMultDepth(ast, multDepthmap)==l + r);
}

int ConeRewriter::getMultDepth(AbstractNode *n) {

  return 0;
}

int ConeRewriter::getReverseMultDepth(MultDepthMap multiplicativeDepthsReversed,
                                      AbstractNode *n) {
  // check if we have calculated the reverse multiplicative depth previously
  if (!multiplicativeDepthsReversed.empty()) {
    auto it = multiplicativeDepthsReversed.find(n->getUniqueNodeId());
    if (it!=multiplicativeDepthsReversed.end())
      return it->second;
  } else {
    throw std::runtime_error("Map is empty!");
  }
}

int ConeRewriter::depthValue(AbstractNode *n) {
  if (auto lexp = dynamic_cast<BinaryExpression *>(n)) {
    // the multiplicative depth considers logical AND nodes only
    return (lexp->getOperator().toString()=="&&");
  }
  return 0;
}

int ConeRewriter::computeMultDepthL(AbstractNode *n, MultDepthMap &multDepthMap) {

  // Only continue if n is non-null
  if (n==nullptr) return 0;

  // check if we have calculated the multiplicative depth previously
  if (!multDepthMap.empty()) {
    auto it = multDepthMap.find(n->getUniqueNodeId());
    if (it!=multDepthMap.end())
      return it->second;
  }

  // next nodes to consider (children)
  std::vector<AbstractNode *> nextNodesToConsider;
  for (auto &v : *n) { nextNodesToConsider.push_back(&v); }

  // we need to compute the multiplicative depth
  // base case: v is a leaf node (input node), i.e., does not have any child node
  // paper: |pred(n)| = 0 => multiplicative depth = 0 + d(n) (here:  |children(n)| = 0 => multdepth = 0 + d(n))
  if (nextNodesToConsider.empty()) {
    return 0 + depthValue(n);
  }

  // otherwise compute max_{u ∈ children(n)} l(u) + d(n)
  int max = 0;
  for (auto &u : nextNodesToConsider) {
    int uDepth;
    // compute the multiplicative depth of child u
    MultDepthMap m;
    uDepth = computeMultDepthL(u, m);
    if (uDepth > max) { max = uDepth; } // update maximum if necessary
  }
  auto r = max + depthValue(n);
  multDepthMap.insert_or_assign(n->getUniqueNodeId(), r);
  return r;
}

int ConeRewriter::computeReversedMultDepthR(AbstractNode *n,
                                            MultDepthMap &multiplicativeDepthsReversedMap,
                                            AbstractNode *root) {

// check if we have calculated the reverse multiplicative depth previously
  if (!multiplicativeDepthsReversedMap.empty()) {
    auto it = multiplicativeDepthsReversedMap.find(n->getUniqueNodeId());
    if (it!=multiplicativeDepthsReversedMap.end())
      return it->second;
  }

  AbstractNode *nextNodeToConsider;

  // if root node, the reverse multiplicative depth is 0
  if (!n->hasParent()) {
    return 0;
  } else {
    nextNodeToConsider = &n->getParent();

  }
  //std::cout << "Considering: " << nextNodeToConsider->toString(false) << " ID: " << nextNodeToConsider->getUniqueNodeId() << std::endl;

  // otherwise compute the reverse depth
  int max = 0;
  int uDepthR;
  uDepthR = computeReversedMultDepthR(nextNodeToConsider, multiplicativeDepthsReversedMap, nullptr);
  if (uDepthR > max) { max = uDepthR; }
  multiplicativeDepthsReversedMap.insert_or_assign(n->getUniqueNodeId(), max + depthValue(nextNodeToConsider));
  return max + depthValue(nextNodeToConsider);
}

int ConeRewriter::getMaximumMultDepth(AbstractNode *root, MultDepthMap map) {
  if (map.empty()) {
    // if map is not empty we dont need to compute it
    throw std::runtime_error("Map is empty");
  }
//  } else {
//      computeMultDepthL(root, map);
//    }
  unsigned currentMax = 0;
  for (auto it = map.cbegin(); it!=map.cend(); ++it) {
    if (it->second > currentMax) {
      currentMax = it->second;
    }
  }
  return currentMax;
}

int getMultDepthL(MultDepthMap multiplicativeDepths, AbstractNode &n) {
  if (multiplicativeDepths.empty()) { return -1; }
  return multiplicativeDepths[n.getUniqueNodeId()];
}

std::vector<AbstractNode *> ConeRewriter::sortTopologically(std::vector<AbstractNode *> &nodes) {

  reverseEdges(nodes);

  std::vector<AbstractNode *> L;
  std::map<AbstractNode *, int> numEdgesDeleted;

  // S <- nodes without an incoming edge
  std::vector<AbstractNode *> S;
  std::for_each(nodes.begin(), nodes.end(), [&](AbstractNode *n) {
    if (n->getParentList().empty()) S.push_back(n);
  });

  while (!S.empty()) {
    auto n = S.back();
    S.pop_back();
    L.push_back(n);
    for (auto &m : n->getChildrenNonNull()) {
      numEdgesDeleted[m] += 1; // emulates removing edge from the graph
      if (m->getParentList().size()==numEdgesDeleted[m]) S.push_back(m);
    }
  }
  return L;
}

std::map<AbstractNode *, float> ConeRewriter::compFlow(std::vector<AbstractNode *> ckt) {
  std::map<AbstractNode *, float> computedFlow;
  std::map<std::pair<AbstractNode *, AbstractNode *>, float> edgeFlows;
  std::cout << "Sorting..." << std::endl;
  auto topologicalOrder = ConeRewriter::sortTopologically(ckt);
  std::cout << "Done Sorting" << std::endl;
  for (AbstractNode *v : topologicalOrder) {
    // if v is input return trivial
    if (v->getChildrenList().empty()) {
      computedFlow[v] = 1;
    } else { // if v is intermediate node
      // compute flow by accumulating flows of incoming edges (u,v) where u ∈ pred(v)
      auto predecessorsOfV = v->getChildrenList();
      float flow = 0.0f;
      std::for_each(predecessorsOfV.begin(), predecessorsOfV.end(), [&](AbstractNode *u) {
        flow += edgeFlows[std::pair(u, v)];
      });
      computedFlow[v] = flow;
    }
    // for all 'successors' (paper) u: define edge flow
    for (auto &u : v->getParentList()) {
      edgeFlows[std::make_pair(v, u)] = computedFlow[v]/v->getParentList().size();
    }
  }
  return computedFlow;
}

void ConeRewriter::reverseEdges(const std::vector<AbstractNode *> &nodes) {
  for (AbstractNode *n : nodes) n->swapChildrenParents();
}

void ConeRewriter::addElements(std::vector<AbstractNode *> &result, std::vector<AbstractNode *> newElements) {
  result.reserve(result.size() + newElements.size());
  result.insert(result.end(), newElements.begin(), newElements.end());
}

void ConeRewriter::flattenVectors(std::vector<AbstractNode *> &resultVector,
                                  std::vector<std::vector<AbstractNode *>> vectorOfVectors) {
  std::set<AbstractNode *> res;
  std::for_each(vectorOfVectors.begin(), vectorOfVectors.end(), [&](std::vector<AbstractNode *> rVec) {
    res.insert(rVec.begin(), rVec.end());
  });
  resultVector.assign(res.begin(), res.end());
}

