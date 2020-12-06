import collections, random
import time
pagesize = 4096

############################################################

# An algorithm that solves an MDP (i.e., computes the optimal
# policy).
class MDPAlgorithm:
    # Set:
    # - self.pi: optimal policy (mapping from state to action)
    # - self.V: values (mapping from state to best values)
    def solve(self, mdp): raise NotImplementedError("Override me")

############################################################
class ValueIteration(MDPAlgorithm):
    '''
    Solve the MDP using value iteration.  Your solve() method must set
    - self.V to the dictionary mapping states to optimal values
    - self.pi to the dictionary mapping states to an optimal action
    Note: epsilon is the error tolerance: you should stop value iteration when
    all of the values change by less than epsilon.
    The ValueIteration class is a subclass of util.MDPAlgorithm (see util.py).
    '''

    def solve(self, mdp, epsilon=0.001):
        begin = time.clock()
        xx = open("x.txt", 'w+')
        FILE = open("out.txt", 'w+')
        mdp.computeStates()
        def computeQ(mdp, V, state, action):
            # Return Q(state, action) based on V(state).
            for newState, prob, reward in mdp.succAndProbReward(state, action):
                #返回值时读取了每个prob、reward、discount、newState和V[newState]
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(prob)/pagesize)) + '\n')
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(reward)/pagesize)) + '\n')
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(newState)/pagesize)) + '\n')
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(mdp.discount())/pagesize)) + '\n')
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(V[newState])/pagesize)) + '\n')
            return sum(prob * (reward + mdp.discount() * V[newState]) \
                            for newState, prob, reward in mdp.succAndProbReward(state, action))

        def computeOptimalPolicy(mdp, V):
            # Return the optimal policy given the values V.
            pi = {}
            for state in mdp.states:
                pi[state] = max((computeQ(mdp, V, state, action), action) for action in mdp.actions(state))[1]
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(state)/pagesize)) + '\n')
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(pi[state])/pagesize)) + '\n')
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(action)/pagesize)) + '\n')
            return pi

        V = collections.defaultdict(float)  # state -> value of state
        numIters = 0
        while True:
            newV = {}
            for state in mdp.states:
                # This evaluates to zero for end states, which have no available actions (by definition)
                newV[state] = max(computeQ(mdp, V, state, action) for action in mdp.actions(state))
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(newV[state])/pagesize)) + '\n')
                xx.write(str(time.clock()-begin)+'\n')
                FILE.write(str(int(id(state)/pagesize)) + '\n')
                for action in mdp.actions(state):
                    xx.write(str(time.clock()-begin)+'\n')
                    FILE.write(str(int(id(action)/pagesize)) + '\n')
            numIters += 1
            xx.write(str(time.clock()-begin)+'\n')
            FILE.write(str(int(id(numIters)/pagesize)) + '\n')
            if max(abs(V[state] - newV[state]) for state in mdp.states) < epsilon:
                V = newV
                break
            V = newV

        # Compute the optimal policy now
        pi = computeOptimalPolicy(mdp, V)
        print(("ValueIteration: %d iterations" % numIters))
        #print(pi)
        self.pi = pi
        self.V = V
        FILE.close()

############################################################
# problem2.2

class PolicyIteration(MDPAlgorithm):
    '''
    Solve the MDP using policy iteration.  Your solve() method must set
    - self.V to the dictionary mapping states to optimal values
    - self.pi to the dictionary mapping states to an optimal action
    Note: epsilon is the error tolerance: you should stop value iteration when
    all of the values change by less than epsilon.
    The PolicyIteration class is a subclass of util.MDPAlgorithm (see util.py).
    '''
    
    def solve(self, mdp, epsilon=0.001):
        # BEGIN_YOUR_CODE
        # raise Exception("Not implemented yet")
        mdp.computeStates()
        policy = {}
        self.numIters = 0
        v = {}
        for s in mdp.states:
            policy[s] = random.choice(mdp.actions(None))
            v[s] = 0

        def computeQ(mdp, V, state, action):
            # Return Q(state, action) based on V(state).
            return sum(prob * (reward + mdp.discount() * V[newState]) \
                            for newState, prob, reward in mdp.succAndProbReward(state, action))

        def extract_policy(v, mdp):
            policy = {}
            actions = mdp.actions(None)
            for s in mdp.states:
                policy[s] = max((computeQ(mdp, v, s, action), action) for action in mdp.actions(s))[1]
                #q_sa = np.zeros(len(mdp.actions(None)))
                #for a in range(len(actions)):
                #    q_sa[a] = computeQ(mdp, v, s, actions[a])
                #policy[s] = actions[np.argmax(q_sa)]
            return policy
            #V = collections.defaultdict(float)
            

        def compute_policy_v(mdp, policy):
            eps = 1e-10
            while True:
                prev_v = v.copy()
                for s in mdp.states:
                    policy_a = policy[s]
                    v[s] = sum([p*(r+mdp.discount()*prev_v[s_]) for s_, p, r in mdp.succAndProbReward(s,policy_a)])
                self.numIters += 1
                if(sum(abs(prev_v[state]-v[state]) for state in mdp.states) <= eps):
                    break
            return v

        max_iteration = 100000
        for _ in range(max_iteration):
            old_policy_v = compute_policy_v(mdp, policy)
            new_policy = extract_policy(old_policy_v, mdp)
            switch = True
            for s in mdp.states:
                if policy[s] != new_policy[s]:
                    switch = False
            if switch:
                break
            policy = new_policy

        #print(("PolicyIteration: %d iterations" % numIters))
        #self.v = v
        self.policy = policy
        print(("PolicyIteration: %d iterations" % self.numIters))
        #print(v)

        #def computeOptimalPolicy(mdp, V):
        # print(("PolicyIteration: %d iterations" % numIters))
        # print(V)
        # self.pi = pi
        # self.V = V
        # END_YOUR_CODE

############################################################
# An abstract class representing a Markov Decision Process (MDP).
class MDP:
    # Return the start state.
    def startState(self): raise NotImplementedError("Override me")

    # Return set of actions possible from |state|.
    def actions(self, state): raise NotImplementedError("Override me")

    # Return a list of (newState, prob, reward) tuples corresponding to edges
    # coming out of |state|.
    # Mapping to notation from class:
    #   state = s, action = a, newState = s', prob = T(s, a, s'), reward = Reward(s, a, s')
    # If IsEnd(state), return the empty list.
    def succAndProbReward(self, state, action): raise NotImplementedError("Override me")

    def discount(self): raise NotImplementedError("Override me")

    # Compute set of states reachable from startState.  Helper function for
    # MDPAlgorithms to know which states to compute values and policies for.
    # This function sets |self.states| to be the set of all states.
    def computeStates(self):
        self.states = set()
        queue = []
        self.states.add(self.startState())
        queue.append(self.startState())
        while len(queue) > 0:
            state = queue.pop()
            for action in self.actions(state):
                for newState, prob, reward in self.succAndProbReward(state, action):
                    if newState not in self.states:
                        self.states.add(newState)
                        queue.append(newState)
        # print "%d states" % len(self.states)
        # print self.states

############################################################

# A simple example of an MDP where states are integers in [-n, +n].
# and actions involve moving left and right by one position.
# We get rewarded for going to the right.
class NumberLineMDP(MDP):
    def __init__(self, n=5): self.n = n
    def startState(self): return 0
    def actions(self, state): return [-1, +1]
    def succAndProbReward(self, state, action):
        return [(state, 0.4, 0),
                (min(max(state + action, -self.n), +self.n), 0.6, state)]
    def discount(self): return 0.9

############################################################




############################################################

# Abstract class: an RLAlgorithm performs reinforcement learning.  All it needs
# to know is the set of available actions to take.  The simulator (see
# simulate()) will call getAction() to get an action, perform the action, and
# then provide feedback (via incorporateFeedback()) to the RL algorithm, so it can adjust
# its parameters.
class RLAlgorithm:
    # Your algorithm will be asked to produce an action given a state.
    def getAction(self, state): raise NotImplementedError("Override me")

    # We will call this function when simulating an MDP, and you should update
    # parameters.
    # If |state| is a terminal state, this function will be called with (s, a,
    # 0, None). When this function is called, it indicates that taking action
    # |action| in state |state| resulted in reward |reward| and a transition to state
    # |newState|.
    def incorporateFeedback(self, state, action, reward, newState): raise NotImplementedError("Override me")

# An RL algorithm that acts according to a fixed policy |pi| and doesn't
# actually do any learning.
class FixedRLAlgorithm(RLAlgorithm):
    def __init__(self, pi): self.pi = pi

    # Just return the action given by the policy.
    def getAction(self, state): return self.pi[state]

    # Don't do anything: just stare off into space.
    def incorporateFeedback(self, state, action, reward, newState): pass

############################################################

# Perform |numTrials| of the following:
# On each trial, take the MDP |mdp| and an RLAlgorithm |rl| and simulates the
# RL algorithm according to the dynamics of the MDP.
# Each trial will run for at most |maxIterations|.
# Return the list of rewards that we get for each trial.
def simulate(mdp, rl, numTrials=10, maxIterations=1000, verbose=False,
             sort=False):
    # Return i in [0, ..., len(probs)-1] with probability probs[i].
    def sample(probs):
        target = random.random()
        accum = 0
        for i, prob in enumerate(probs):
            accum += prob
            if accum >= target: return i
        raise Exception("Invalid probs: %s" % probs)

    totalRewards = []  # The rewards we get on each trial
    for trial in range(numTrials):
        state = mdp.startState()
        sequence = [state]
        totalDiscount = 1
        totalReward = 0
        for _ in range(maxIterations):
            action = rl.getAction(state)
            transitions = mdp.succAndProbReward(state, action)
            if sort: transitions = sorted(transitions)
            if len(transitions) == 0:
                rl.incorporateFeedback(state, action, 0, None)
                break

            # Choose a random transition
            i = sample([prob for newState, prob, reward in transitions])
            newState, prob, reward = transitions[i]
            sequence.append(action)
            sequence.append(reward)
            sequence.append(newState)

            rl.incorporateFeedback(state, action, reward, newState)
            totalReward += totalDiscount * reward
            totalDiscount *= mdp.discount()
            state = newState
        if verbose:
            print(("Trial %d (totalReward = %s): %s" % (trial, totalReward, sequence)))
        totalRewards.append(totalReward)
    return totalRewards
