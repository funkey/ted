import pyted
import numpy as np

def create_test_data(background_in_a = True, background_in_b = True, randomized = False):

    size = 10

    if background_in_a:
        a = np.zeros((size,size,size), dtype=np.uint32)
    else:
        a = np.ones((size,size,size), dtype=np.uint32)

    if background_in_b:
        b = np.zeros((size,size,size), dtype=np.uint32)
    else:
        b = np.ones((size,size,size), dtype=np.uint32)

    if randomized:
        a += np.array(np.random.rand(size, size, size)*3, dtype=np.uint32)
        b += np.array(np.random.rand(size, size, size)*3, dtype=np.uint32)
    else:
        a[:,size/2:size,:] = 2
        b[size/4:size,:,:] = 2

    return (a,b)

def test(ba, bb):

    print
    print("groundtruth    with background: " + str(ba))
    print("reconstruction with background: " + str(bb))
    print

    (a,b) = create_test_data(background_in_a = ba, background_in_b = bb, randomized = True)

    parameters = pyted.Parameters()
    parameters.report_ted  = True
    parameters.report_rand = True
    parameters.report_voi  = True
    parameters.from_skeleton = True
    parameters.distance_threshold = 10
    parameters.gt_background_label = 0.0
    ted = pyted.Ted(parameters)
    report = ted.create_report(a, b)

    print("TED report:")
    for (k,v) in report.iteritems():
        print("\t" + k.ljust(20) + ": " + str(v))

if __name__ == "__main__":

    for ba in [True, False]:
        for bb in [True, False]:
            test(ba, bb)
