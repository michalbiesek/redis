start_server {tags {"defrag"}} {
    if {[string match {*jemalloc*} [s mem_allocator]] || [s mem_allocator] == "memkind"} {

        test "Active defrag big keys" {
            r flushdb
            r config resetstat
            r config set save "" ;# prevent bgsave from interfereing with save below
            r config set hz 100
            r config set activedefrag no
            r config set active-defrag-max-scan-fields 1000
            r config set active-defrag-threshold-lower 5
            r config set active-defrag-cycle-min 65
            r config set active-defrag-cycle-max 75
            r config set active-defrag-ignore-bytes 2mb
            r config set maxmemory 0
            r config set list-max-ziplist-size 5 ;# list of 10k items will have 2000 quicklist nodes
            r config set stream-node-max-entries 5
            r hmset hash h1 v1 h2 v2 h3 v3
            r lpush list a b c d
            r zadd zset 0 a 1 b 2 c 3 d
            r sadd set a b c d
            r xadd stream * item 1 value a
            r xadd stream * item 2 value b
            r xgroup create stream mygroup 0
            r xreadgroup GROUP mygroup Alice COUNT 1 STREAMS stream >

            # create big keys with 10k items
            set rd [redis_deferring_client]
            for {set j 0} {$j < 10000} {incr j} {
                $rd hset bighash $j [concat "asdfasdfasdf" $j]
                $rd lpush biglist [concat "asdfasdfasdf" $j]
                $rd zadd bigzset $j [concat "asdfasdfasdf" $j]
                $rd sadd bigset [concat "asdfasdfasdf" $j]
                $rd xadd bigstream * item 1 value a
            }
            for {set j 0} {$j < 50000} {incr j} {
                $rd read ; # Discard replies
            }

            set expected_frag 1.7
            if {$::accurate} {
                # scale the hash to 1m fields in order to have a measurable the latency
                for {set j 10000} {$j < 1000000} {incr j} {
                    $rd hset bighash $j [concat "asdfasdfasdf" $j]
                }
                for {set j 10000} {$j < 1000000} {incr j} {
                    $rd read ; # Discard replies
                }
                # creating that big hash, increased used_memory, so the relative frag goes down
                set expected_frag 1.3
            }

            # add a mass of string keys
            for {set j 0} {$j < 500000} {incr j} {
                $rd setrange $j 150 a
            }
            for {set j 0} {$j < 500000} {incr j} {
                $rd read ; # Discard replies
            }
            assert_equal [r dbsize] 500010

            # create some fragmentation
            for {set j 0} {$j < 500000} {incr j 2} {
                $rd del $j
            }
            for {set j 0} {$j < 500000} {incr j 2} {
                $rd read ; # Discard replies
            }
            assert_equal [r dbsize] 250010

            # start defrag
            after 120 ;# serverCron only updates the info once in 100ms
            set frag [s allocator_frag_ratio]
            if {$::verbose} {
                puts "frag $frag"
            }
            assert {$frag >= $expected_frag}
            r config set latency-monitor-threshold 5
            r latency reset

            set digest [r debug digest]
            catch {r config set activedefrag yes} e
            if {![string match {DISABLED*} $e]} {
                # wait for the active defrag to start working (decision once a second)
                wait_for_condition 50 100 {
                    [s active_defrag_running] ne 0
                } else {
                    fail "defrag not started."
                }

                # wait for the active defrag to stop working
                wait_for_condition 500 100 {
                    [s active_defrag_running] eq 0
                } else {
                    after 120 ;# serverCron only updates the info once in 100ms
                    puts [r info memory]
                    puts [r memory malloc-stats]
                    fail "defrag didn't stop."
                }

                # test the the fragmentation is lower
                after 120 ;# serverCron only updates the info once in 100ms
                set frag [s allocator_frag_ratio]
                set max_latency 0
                foreach event [r latency latest] {
                    lassign $event eventname time latency max
                    if {$eventname == "active-defrag-cycle"} {
                        set max_latency $max
                    }
                }
                if {$::verbose} {
                    puts "frag $frag"
                    puts "max latency $max_latency"
                    puts [r latency latest]
                    puts [r latency history active-defrag-cycle]
                }
                assert {$frag < 1.1}
                # due to high fragmentation, 100hz, and active-defrag-cycle-max set to 75,
                # we expect max latency to be not much higher than 7.5ms but due to rare slowness threshold is set higher
                assert {$max_latency <= 30}
            }
            # verify the data isn't corrupted or changed
            set newdigest [r debug digest]
            assert {$digest eq $newdigest}
            r save ;# saving an rdb iterates over all the data / pointers
        } {OK}
    }
}
