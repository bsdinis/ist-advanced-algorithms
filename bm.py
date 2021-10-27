#!/usr/bin/env python3

NO_OF_CHARS = 256

def badCharHeuristic(string, size):
    '''
    The preprocessing function for
    Boyer Moore's bad character heuristic
    '''

    # Initialize all occurrence as -1
    badChar = [-1]*NO_OF_CHARS

    # Fill the actual value of last occurrence
    for i in range(size):
        badChar[ord(string[i])] = i

    # retun initialized list
    return badChar

# preprocessing for strong good suffix rule
def preprocess_strong_suffix(shift, bpos, pat, m):

	# m is the length of pattern
	i = m
	j = m + 1
	bpos[i] = j

	while i > 0:

		'''if character at position i-1 is
		not equivalent to character at j-1,
		then continue searching to right
		of the pattern for border '''
		while j <= m and pat[i - 1] != pat[j - 1]:

			''' the character preceding the occurrence
			of t in pattern P is different than the
			mismatching character in P, we stop skipping
			the occurrences and shift the pattern
			from i to j '''
			if shift[j] == 0:
				shift[j] = j - i

			# Update the position of next border
			j = bpos[j]

		''' p[i-1] matched with p[j-1], border is found.
		store the beginning position of border '''
		i -= 1
		j -= 1
		bpos[i] = j

# Preprocessing for case 2
def preprocess_case2(shift, bpos, pat, m):
	j = bpos[0]
	for i in range(m + 1):

		''' set the border position of the first character
		of the pattern to all indices in array shift
		having shift[i] = 0 '''
		if shift[i] == 0:
			shift[i] = j

		''' suffix becomes shorter than bpos[0],
		use the position of next widest border
		as value of j '''
		if i == j:
			j = bpos[j]

'''Search for a pattern in given text using
Boyer Moore algorithm with Good suffix rule '''
def search(text, pat):

	# s is shift of the pattern with respect to text
	s = 0
	m = len(pat)
	n = len(text)
	c = 0


	bpos = [0] * (m + 1)

	# initialize all occurrence of shift to 0
	shift = [0] * (m + 1)
	out = ""


	# do preprocessing
	preprocess_strong_suffix(shift, bpos, pat, m)
	preprocess_case2(shift, bpos, pat, m)
	print("Shift ("+str(len(shift))+") = "+str(shift)+"\n")
	badChar = badCharHeuristic(pat, m)

	while s <= n - m:
		j = m - 1

		''' Keep reducing index j of pattern while characters of
			pattern and text are matching at this shift s'''
		while j >= 0 and pat[j] == text[s + j]:
			c+=1
			j -= 1

		''' If the pattern is present at the current shift,
			then index j will become -1 after the above loop '''
		if j < 0:
			out += str(s) + " "
			s += shift[0]
		else:

			'''pat[i] != pat[s+j] so shift the pattern
			shift[j+1] times '''

			# Comment next line and uncomment the line after for no Bad Character usage

			s += max(shift[j + 1], j-badChar[ord(text[s+j])])
			# s += shift[j+1]


			c+=1

	out+= "\n" + str(c) +" \n"
	print(out)

# Driver Code
if __name__ == "__main__":
	line = input()
	text = ""
	pat = ""
	while line[0] != 'X':
		if line[0] == 'T':
			text = line[2:]
		elif line[0] == 'B':
			pat = line[2:]
			search(text,pat)
		line = input()

# This code is contributed by
# sanjeev2552
