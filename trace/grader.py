#!/usr/bin/env python3

import random, util, collections
import graderUtil

grader = graderUtil.Grader()
submission = grader.load('submission')

############################################################
# test 2.1 & 2.2
def test3a():
    mdp1 = submission.BlackjackMDP(cardValues=[1, 5], multiplicity=2,
                                   threshold=10, peekCost=1)
    startState = mdp1.startState()
    preBustState = (6, None, (1, 1))
    postBustState = (11, None, None)

    mdp2 = submission.BlackjackMDP(cardValues=[1, 5], multiplicity=2,
                                   threshold=15, peekCost=1)
    preEmptyState = (11, None, (1,0))

    # Make sure the succAndProbReward function is implemented correctly.
    tests = [
        ([((1, None, (1, 2)), 0.5, 0), ((5, None, (2, 1)), 0.5, 0)], mdp1, startState, 'Take'),
        ([((0, 0, (2, 2)), 0.5, -1), ((0, 1, (2, 2)), 0.5, -1)], mdp1, startState, 'Peek'),
        ([((0, None, None), 1, 0)], mdp1, startState, 'Quit'),
        ([((7, None, (0, 1)), 0.5, 0), ((11, None, None), 0.5, 0)], mdp1, preBustState, 'Take'),
        ([], mdp1, postBustState, 'Take'),
        ([], mdp1, postBustState, 'Peek'),
        ([], mdp1, postBustState, 'Quit'),
        ([((12, None, None), 1, 12)], mdp2, preEmptyState, 'Take')
    ]
    for gold, mdp, state, action in tests:
        if not grader.requireIsEqual(gold,
                                     mdp.succAndProbReward(state, action)):
            print(('   state: {}, action: {}'.format(state, action)))
#grader.addBasicPart('3a-basic', test3a, 5, description="Basic test for succAndProbReward() that covers several edge cases.")

def test3aHidden():
    mdp = submission.BlackjackMDP(cardValues=[1, 3, 5, 8, 10], multiplicity=3,
                                  threshold=40, peekCost=1)
    # value iteration
    startState = mdp.startState()

    alg = util.ValueIteration()
    alg.solve(mdp, .0001)
    # policy iteration
    # startState = mdp.startState()
    # alg = util.PolicyIteration()
    # alg.solve(mdp, .0001)
#grader.addHiddenPart('3a-hidden', test3aHidden, 5, description="Hidden test for ValueIteration. Run ValueIteration on BlackjackMDP, then test if V[startState] is correct.")

###########################################################
# test 2.3
def test3b():
    mdp = submission.peekingMDP()
    vi = submission.ValueIteration()
    vi.solve(mdp)
    grader.requireIsEqual(mdp.threshold, 20)
    grader.requireIsEqual(mdp.peekCost, 1)
    nonTerminateStates = {k:v for k, v in vi.pi.items() if k[2] is not None}
    f = len([a for a in list(nonTerminateStates.values()) if a == 'Peek']) / float(len(list(nonTerminateStates.values())))
    grader.requireIsGreaterThan(.1, f)
    # Feel free to uncomment these lines if you'd like to print out states/actions
    # for k, v in vi.pi.iteritems():
    #     print k, v
grader.addBasicPart('3b-basic', test3b, 4, description="Test for peekingMDP().  Ensure that in at least 10% of states, the optimal policy is to peek.")

grader.grade()
