import time

# returns the current time in microseconds since the start of the day
def day_micro():
    return int(time.time() % 86400 * 1_000_000)
    #as a notice, if you test lightdance over midnight, you have to run it again.