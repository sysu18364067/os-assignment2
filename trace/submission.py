import util, math, random
from collections import defaultdict
from util import ValueIteration

############################################################
# Problem 2.1

class BlackjackMDP(util.MDP):
    def __init__(self, cardValues, multiplicity, threshold, peekCost):
        """
        cardValues: list of integers (face values for each card included in the deck)
        multiplicity: single integer representing the number of cards with each face value
        threshold: maximum number of points (i.e. sum of card values in hand) before going bust
        peekCost: how much it costs to peek at the next card
        """
        self.cardValues = cardValues
        self.multiplicity = multiplicity
        self.threshold = threshold
        self.peekCost = peekCost

    # Return the start state.
    # Look closely at this function to see an example of state representation for our Blackjack game.
    # Each state is a tuple with 3 elements:
    #   -- The first element of the tuple is the sum of the cards in the player's hand.
    #   -- If the player's last action was to peek, the second element is the index
    #      (not the face value) of the next card that will be drawn; otherwise, the
    #      second element is None.
    #   -- The third element is a tuple giving counts for each of the cards remaining
    #      in the deck, or None if the deck is empty or the game is over (e.g. when
    #      the user quits or goes bust).
    def startState(self):
        return (0, None, (self.multiplicity,) * len(self.cardValues))

    # Return set of actions possible from |state|.
    # You do not need to modify this function.
    # All logic for dealing with end states should be placed into the succAndProbReward function below.
    def actions(self, state):
        return ['Take', 'Peek', 'Quit']

    # Given a |state| and |action|, return a list of (newState, prob, reward) tuples
    # corresponding to the states reachable from |state| when taking |action|.
    # A few reminders:
    # * Indicate a terminal state (after quitting, busting, or running out of cards)
    #   by setting the deck to None.
    # * If |state| is an end state, you should return an empty list [].
    # * When the probability is 0 for a transition to a particular new state,
    #   don't include that state in the list returned by succAndProbReward.
    def succAndProbReward(self, state, action):
        # BEGIN_YOUR_CODE (our solution is 38 lines of code, but don't worry if you deviate from this)
        #raise Exception("Not implemented yet")                                         
        nextStates = []
        if state[2] == None or (state[1] != None and action == 'Peek'):                                    #若游戏已经结束或已经完成Peek，返回[]
            pass
        elif action == 'Quit':
            nextStates.append(((state[0], None, None), 1, state[0]))
        elif action == 'Peek':
            sum_s = sum(state[2])
            for i in range(len(state[2])):
                if state[2][i] == 0:                                                #不可能事件，跳过
                    continue
                nextStates.append(((state[0], i, state[2]), state[2][i]/sum_s, -self.peekCost))
        elif action == 'Take':
            sum_s = sum(state[2])
            if state[1] != None:
                if self.cardValues[state[1]] + state[0] > self.threshold:
                    nextStates.append(((state[0]+self.cardValues[state[1]], None, None), 1, 0))
                else:
                    newlist = list(state[2][:])
                    newlist[state[1]] -= 1
                    if sum(newlist) == 0:
                        newlist = None
                    nextStates.append(((state[0]+self.cardValues[state[1]], None, tuple(newlist)), 1, 0))
            else: 
                for i in range(len(state[2])):
                    if state[2][i] == 0:
                        continue
                    elif self.cardValues[i] + state[0] > self.threshold:
                        nextStates.append(((state[0]+self.cardValues[i], None, None), state[2][i]/sum_s, 0))
                    elif(sum(state[2]) == 1):
                        nextStates.append(((state[0]+self.cardValues[i], None, None), 1, state[0]+self.cardValues[i]))
                    else:
                        newlist = list(state[2][:])
                        newlist[i] -= 1
                        nextStates.append(((state[0]+self.cardValues[i], None, tuple(newlist)), state[2][i]/sum_s, 0))
        return nextStates
        # END_YOUR_CODE

    def discount(self):
        return 1

############################################################
# Problem 2.3

def peekingMDP():
    """
    Return an instance of BlackjackMDP where peeking is the
    optimal action at least 10% of the time.
    """
    # BEGIN_YOUR_CODE (our solution is 2 lines of code, but don't worry if you deviate from this)
    #mdp = BlackjackMDP(cardValues=[5,20], multiplicity=6, threshold=20, peekCost=1)
    mdp = BlackjackMDP(cardValues=[2, 3, 25], multiplicity=20, threshold=20, peekCost=1)
    return mdp
    #raise Exception("Not implemented yet")
    # END_YOUR_CODE


