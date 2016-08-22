import pyted
import numpy as np
import cremi

def create_test_data(background_in_a = True, background_in_b = True, randomized = False):

    if background_in_a:
        a = np.zeros((100,100,100), dtype=np.uint32)
    else:
        a = np.ones((100,100,100), dtype=np.uint32)

    if background_in_b:
        b = np.zeros((100,100,100), dtype=np.uint32)
    else:
        b = np.ones((100,100,100), dtype=np.uint32)

    if randomized:
        a += np.array(np.random.rand(100, 100, 100)*3, dtype=np.uint32)
        b += np.array(np.random.rand(100, 100, 100)*3, dtype=np.uint32)
    else:
        a[:,50:100,:] = 2
        b[25:100,:,:] = 2

    return (a,b)

def test(ba, bb):

    print("Testing evaluation")
    print("==================")
    print
    print("groundtruth    with background: " + str(ba))
    print("reconstruction with background: " + str(bb))

    (a,b) = create_test_data(background_in_a = ba, background_in_b = bb, randomized = True)

    av = cremi.Volume(a)
    bv = cremi.Volume(b)

    evaluate = cremi.evaluation.NeuronIds(av)
    cremi_result_voi = evaluate.voi(bv)
    cremi_result_rand = evaluate.adapted_rand(bv)

    t = pyted.Ted()
    ted_result = t.create_report(a, b)

    print("CREMI:")
    print("\tvoi split   : " + str(cremi_result_voi[0]))
    print("\tvoi merge   : " + str(cremi_result_voi[1]))
    print("\tadapted Rand: " + str(cremi_result_rand))
    print("TED  :")
    print("\tvoi split   : " + str(ted_result['voi_split']))
    print("\tvoi merge   : " + str(ted_result['voi_merge']))
    print("\tadapted Rand: " + str(ted_result['adapted_rand_error']))

for ba in [True, False]:
    for bb in [True, False]:
        test(ba, bb)
