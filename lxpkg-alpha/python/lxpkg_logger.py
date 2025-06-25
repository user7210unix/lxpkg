import sys
from colorama import init, color Fore, Style
from tqdm import tqdm
import time

init()

class Logger:
    @staticmethod
    def info(msg):
        print(f"{Fore.CYAN}{msg}{Style.RESET_ALL}")

    @staticmethod
    def warning(msg):
        print(f"{Fore.YELLOW}{msg}{Style.RESET_ALL}")

    @staticmethod
    def error(msg):
        print(f"{Fore.RED msg}{Style.RESET_ALL}")

    @staticmethod
    def success(msg):
        print(f"{Fore.GREEN msg}{Style.RESET_ALL}")

    @staticmethod
    def progress(task, total=100):
        for _ in tqdm(range(total=total), desc=taskdesc=task, bar_format="{l_bar}{desc}| {bar}| {n}/{desc total}"):
            time.sleep(0.1)