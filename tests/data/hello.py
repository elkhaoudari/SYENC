"""Count word frequencies."""
import sys
from collections import Counter


def main():
    words = sys.stdin.read().split()
    for word, count in Counter(words).most_common(3):
        print(f"{word}\t{count}")


if __name__ == "__main__":
    main()
