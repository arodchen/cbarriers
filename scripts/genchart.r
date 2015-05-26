#Copyright 2015 Andrey Rodchenko, School of Computer Science, The University of Manchester
#
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

options( echo=TRUE, error=recover)
library(gplots)
library(data.table)
syntheticBenchmarks <- c("pure","ldimbl","sanity")
syntheticAxisName <- paste("Nanoseconds per Barrier")
realAxisName <- paste("Seconds")
syntheticTitleEntity <- paste("Overhead")
realTitleEntity <- paste("Performance")
pathSeparator <- paste("/")
cfgSeparator <- paste("__")
surSubDir <- paste("sur")
# can in be changed in range [50:72]
barStepMin <- 50
barStepMax <- 72
plotStep <- 36
threadAnnAdj <- -0.3
# args vector which can be used for debugging
# args <- c( "testdb", "charts", "0", "ignoreBarriers=", "surOnlySpinnings=", "interpolateRadix=no")
args <- commandArgs( trailingOnly = TRUE)
dbDir <- paste( args [ 1 ])
chartDir <- paste( args [ 2 ])
threadsSurplus <- paste( args [ 3 ])
ignoreBarriers <- strsplit( gsub( "ignoreBarriers=", "", gsub( " ", "", args [ 4 ])), ",")[[1]]
surOnlySpinnings <- strsplit( gsub( "surOnlySpinnings=", "", gsub( " ", "", args [ 5 ])), ",")[[1]]
interpolateRadix <- ifelse( gsub( "interpolateRadix=", "", args [ 6 ]) == "yes", TRUE, FALSE)
globDb <- read.table( paste( dbDir,"/test.db", sep = ""), header = TRUE, sep = ",")

globDb$Affinity <- NULL
globDb$Nanoseconds.per.Synchronized.Phase <- NULL
globDb$Nanoseconds.per.Unsynchronized.Phase <- NULL

geomeanFunc = function( x) { exp( mean( log( x))) }
#geomeanFunc = function( x) { prod( x) ^ (1 / length( x)) }

plotFunc <- boxplot
#plotFunc <- plotmeans

annotateBestWorstValuesFunc <- function( bwTDb, curTLB, curTHB)
{
    minNanoseconds <- min( bwTDb$Nanoseconds.per.Barrier)
    maxNanoseconds <- max( bwTDb$Nanoseconds.per.Barrier)
    dltNanoseconds <- maxNanoseconds - minNanoseconds
    adjD  <- 0.1 * dltNanoseconds
    subBWTDb <- bwTDb [ curTLB : curTHB, ]
    lD <- length( subBWTDb$Threads.Number)
    if ( dltNanoseconds == 0 )
    {
        adjText <- rep( 1, lD)
    } else
    {
        adjText <- ifelse( ((subBWTDb$Nanoseconds.per.Barrier - minNanoseconds)/dltNanoseconds) < 0.5, 0, 1)
    }
    for ( i8 in 1:lD )
    {
        text( i8 + threadAnnAdj - (curTLB - 1),
              subBWTDb$Nanoseconds.per.Barrier[ i8 ] + adjD * (1 - (2 * adjText[ i8 ])), 
              paste( subBWTDb$Barrier[ i8 ],
                     subBWTDb$Spinning[ i8 ], 
                     subBWTDb$Radix[ i8 ], 
                     sep = cfgSeparator),
              srt = 90,
              cex = min( 24 / (curTHB + 1 - curTLB), 1),
              ylim = c( 0, maxNanoseconds),
              adj = adjText[ i8 ])
    }
}

if ( length( ignoreBarriers) != 0 )
{
    iB <- length( unique( ignoreBarriers))
    for ( i9 in 1:iB )
    {
        globDb <- globDb [ globDb$Barrier != unique( ignoreBarriers)[ i9 ], ] 
        globDb$Barrier <- factor( globDb$Barrier)
    }
}

globMeanTimeDb <- aggregate( Nanoseconds.per.Barrier ~ 
                             Hostname + 
                             Architecture + 
                             Experiment.Number + 
                             Benchmark + 
                             Barrier + 
                             Radix + 
                             Spinning + 
                             Threads.Number, 
                             data=globDb,
                             mean)

globBestRadixTimeDb <- aggregate( Nanoseconds.per.Barrier ~ 
                                  Hostname + 
                                  Architecture + 
                                  Experiment.Number + 
                                  Benchmark + 
                                  Barrier + 
                                  Spinning + 
                                  Threads.Number, 
                                  data=globMeanTimeDb, 
                                  min)
globBestRadixTimeDb <- merge( globBestRadixTimeDb, globMeanTimeDb)


globWorstRadixTimeDb <- aggregate( Nanoseconds.per.Barrier ~ 
                                   Hostname + 
                                   Architecture + 
                                   Experiment.Number + 
                                   Benchmark + 
                                   Barrier + 
                                   Spinning + 
                                   Threads.Number, 
                                   data=globMeanTimeDb, 
                                   max)
globWorstRadixTimeDb <- merge( globWorstRadixTimeDb, globMeanTimeDb)


calcBestWorstTimeDbFunc <- function( argDb, func)
{
    retDb <- aggregate( Nanoseconds.per.Barrier ~ 
                        Hostname + 
                        Architecture + 
                        Experiment.Number + 
                        Benchmark + 
                        Threads.Number, 
                        data=argDb, 
                        func)
    retDb <- merge( retDb, argDb)
}

calcBestTimeDbFunc <- function( argDb)
{
    argDb <- calcBestWorstTimeDbFunc( argDb, min)
}

calcWorstTimeDbFunc <- function( argDb)
{
    argDb <- calcBestWorstTimeDbFunc( argDb, max)
}


hostnameF <- factor( globDb$Hostname)
for ( i1 in 1:length( unique( hostnameF)) )
{
    hostname <- ( unique( hostnameF)) [ i1 ]

    for ( curSur in 0:1 )
    {
        threadsSurplusN <- as.numeric( threadsSurplus)
        if ( ((curSur == 0) && (threadsSurplusN < 1)) ||
             ((curSur == 1) && (threadsSurplusN > 0)) )
        {
            if ( (threadsSurplusN > 0) && length( surOnlySpinnings) )
            {
                surIgnoreSpinnings <- unique( factor( globMeanTimeDb$Spinning))
                surIgnoreSpinnings <- surIgnoreSpinnings [ ! surIgnoreSpinnings %in% surOnlySpinnings ]
                db <- globDb
                meanTimeDb <- globMeanTimeDb
                bestRadixTimeDb <- globBestRadixTimeDb
                worstRadixTimeDb <- globWorstRadixTimeDb
                for ( i10 in 1:length( surIgnoreSpinnings))
                {
                    curSpinning <- surIgnoreSpinnings[ i10 ]
                    curSpinning <- levels( curSpinning)[ as.integer( curSpinning) ]
                    db <- db [ db$Spinning != curSpinning, ]
                    meanTimeDb <- meanTimeDb [ meanTimeDb$Spinning != curSpinning, ]
                    bestRadixTimeDb <- bestRadixTimeDb [ bestRadixTimeDb$Spinning != curSpinning, ]
                    worstRadixTimeDb <- worstRadixTimeDb [ worstRadixTimeDb$Spinning != curSpinning, ]
                }
                bestTimeDb <- calcBestTimeDbFunc( meanTimeDb)
                worstTimeDb <- calcWorstTimeDbFunc( meanTimeDb)
            } else
            {
                db <- globDb
                meanTimeDb <- globMeanTimeDb
                bestRadixTimeDb <- globBestRadixTimeDb
                worstRadixTimeDb <- globWorstRadixTimeDb
                bestTimeDb <- calcBestTimeDbFunc( meanTimeDb)
                worstTimeDb <- calcWorstTimeDbFunc( meanTimeDb)
            }
        }
        if ( ((curSur == 0) && (threadsSurplusN > 0)) )
        {   
            hostDb <- globDb [ globDb$Hostname == hostname, ]
            hostMaxThreadsN <- as.numeric( max( hostDb$Threads.Number))
            upperThreadsBound <- hostMaxThreadsN - threadsSurplusN
            stopifnot( upperThreadsBound > 0)

            db <- globDb [ globDb$Threads.Number <= upperThreadsBound, ]
            meanTimeDb <- globMeanTimeDb [ globMeanTimeDb$Threads.Number <= upperThreadsBound, ]
            bestRadixTimeDb <- globBestRadixTimeDb [ globBestRadixTimeDb$Threads.Number <= upperThreadsBound, ]
            worstRadixTimeDb <- globWorstRadixTimeDb [ globWorstRadixTimeDb$Threads.Number <= upperThreadsBound, ]
            bestTimeDb <- calcBestTimeDbFunc( meanTimeDb)
            worstTimeDb <- calcWorstTimeDbFunc( meanTimeDb)
        }
        if ( (curSur == 1) && (threadsSurplusN < 1) )
        {
            break;
        }
        if ( curSur == 0 )
        {
            hostChartDir <- paste( chartDir, hostname, sep = pathSeparator)
        }
        if ( curSur == 1 )
        {
            hostChartDir <- paste( hostChartDir, surSubDir, sep = pathSeparator)
        }
        if ( !file.exists( hostChartDir) )
        {
            dir.create( hostChartDir);
        }

        subDb1 <- db [ db$Hostname == hostname, ]
        archF <- factor( subDb1$Architecture)
        stopifnot( length( unique( archF)) == 1)
        arch <- (unique( archF)) [ 1 ]

        experimentF <- factor( subDb1$Experiment)
        for ( i2 in 1:length( unique( experimentF)) )
        {
            experiment <- ( unique( experimentF)) [ i2 ]
            subDb2 <- subDb1 [ subDb1$Experiment == experiment, ]
            benchmarkF <- factor( subDb2$Benchmark)

            for ( i3 in 1:length( unique( benchmarkF)) )
            {
                benchmark <- ( unique( benchmarkF)) [ i3 ]
                titleEntity <- ifelse( benchmark %in% syntheticBenchmarks, syntheticTitleEntity, realTitleEntity)
                yAxisName <- ifelse( benchmark %in% syntheticBenchmarks, syntheticAxisName, realAxisName)
                subDb3 <- subDb2 [ subDb2$Benchmark == benchmark, ]

                {
                    subBestTimeDb <- bestTimeDb [ (bestTimeDb$Hostname == hostname), ]
                    subBestTimeDb <- subBestTimeDb [ (subBestTimeDb$Experiment == experiment), ]
                    subBestTimeDb <- subBestTimeDb [ (subBestTimeDb$Benchmark == benchmark), ]
                    subBestTimeDb <- subBestTimeDb [ order( subBestTimeDb$Threads.Number), ]
                    subBestTimeDb <- subBestTimeDb [ !duplicated( subBestTimeDb$Threads.Number), ]
                    subBestTimeDb$compactNumbering <- 1:nrow(subBestTimeDb) 
                    subBestTimePDb <- subBestTimeDb
                    subBestTimePDb$Nanoseconds.per.Barrier <- NULL
                    subBestTimePDb <- merge( subBestTimePDb, subDb3)
                    subBestTimePDb <- subBestTimePDb [ order( subBestTimePDb$Threads.Number), ]

                    subWorstTimeDb <- worstTimeDb [ (worstTimeDb$Hostname == hostname), ]
                    subWorstTimeDb <- subWorstTimeDb [ (subWorstTimeDb$Experiment == experiment), ]
                    subWorstTimeDb <- subWorstTimeDb [ (subWorstTimeDb$Benchmark == benchmark), ]
                    subWorstTimeDb <- subWorstTimeDb [ order( subWorstTimeDb$Threads.Number), ]
                    subWorstTimeDb <- subWorstTimeDb [ !duplicated( subWorstTimeDb$Threads.Number), ]
                    subWorstTimeDb$compactNumbering <- 1:nrow(subWorstTimeDb) 
                    subWorstTimePDb <- subWorstTimeDb
                    subWorstTimePDb$Nanoseconds.per.Barrier <- NULL
                    subWorstTimePDb <- merge( subWorstTimePDb, subDb3)
                    subWorstTimePDb <- subWorstTimePDb [ order( subWorstTimePDb$Threads.Number), ]

                    subMeanTimeDb <- meanTimeDb [ (meanTimeDb$Hostname == hostname), ]
                    subMeanTimeDb <- subMeanTimeDb [ (subMeanTimeDb$Experiment == experiment), ]
                    subMeanTimeDb <- subMeanTimeDb [ (subMeanTimeDb$Benchmark == benchmark), ]
                }

                barrierF <- factor( subDb3$Barrier)
                for ( i4 in 1:length( unique( barrierF)) )
                {
                    barrier <- ( unique( barrierF)) [ i4 ]
                    subDb4 <- subDb3 [ subDb3$Barrier == barrier, ]

                    spinningF <- factor( subDb4$Spinning)
                    for ( i5 in 1:length( unique( spinningF)))
                    {
                        spinning <- ( unique( spinningF)) [ i5 ]
                        spinning <- levels( spinning)[ as.integer( spinning) ]

                        subDb5 <- subDb4 [ subDb4$Spinning == spinning, ]
                        if ( nrow( subDb5) > 0 )
                        {
                            cfg <- paste( hostname, barrier, spinning, benchmark, experiment, sep = cfgSeparator)
                            pdf( paste( hostChartDir, paste( cfg, "pdf", sep = "."), sep = pathSeparator))

                            cfgFile <- paste( paste( hostname, arch, experiment, sep = cfgSeparator),".cfg", sep = "")
                            cfgFile <- paste( dbDir, cfgFile, sep = pathSeparator)
                            if ( file.exists( cfgFile) )
                            {
                                cfgInfo <- read.table( cfgFile, sep = "\n")
                                infoLines <- nrow( cfgInfo)
                                plot( 0:infoLines, type = "n", xaxt="n", yaxt="n", bty="n", xlab = "", ylab = "")
                                for ( iLine in 1:infoLines )
                                {
                                    text( 4, infoLines - iLine, cfgInfo [ iLine, 1], adj = c( 0, NA))
                                }
                            }

                            subBestRadixTimeDb <- bestRadixTimeDb [ (bestRadixTimeDb$Hostname == hostname), ]
                            subBestRadixTimeDb <- subBestRadixTimeDb [ (subBestRadixTimeDb$Spinning == spinning), ]
                            subBestRadixTimeDb <- subBestRadixTimeDb [ (subBestRadixTimeDb$Barrier == barrier), ]
                            subBestRadixTimeDb <- subBestRadixTimeDb [ (subBestRadixTimeDb$Benchmark == benchmark), ]
                            subBestRadixTimeDb <- subBestRadixTimeDb [ (subBestRadixTimeDb$Experiment == experiment), ]
                            subBestRadixTimeDb <- subBestRadixTimeDb [ order( subBestRadixTimeDb$Threads.Number), ]
                            subBestRadixTimeDb <- subBestRadixTimeDb [ !duplicated( subBestRadixTimeDb$Threads.Number), ]
                            subBestRadixTimeDb$compactNumbering <- 1:nrow(subBestRadixTimeDb)

                            subWorstRadixTimeDb <- worstRadixTimeDb [ (worstRadixTimeDb$Hostname == hostname), ]
                            subWorstRadixTimeDb <- subWorstRadixTimeDb [ (subWorstRadixTimeDb$Spinning == spinning), ]
                            subWorstRadixTimeDb <- subWorstRadixTimeDb [ (subWorstRadixTimeDb$Barrier == barrier), ]
                            subWorstRadixTimeDb <- subWorstRadixTimeDb [ (subWorstRadixTimeDb$Benchmark == benchmark), ]
                            subWorstRadixTimeDb <- subWorstRadixTimeDb [ (subWorstRadixTimeDb$Experiment == experiment), ]
                            subWorstRadixTimeDb <- subWorstRadixTimeDb [ order( subWorstRadixTimeDb$Threads.Number), ]
                            subWorstRadixTimeDb <- subWorstRadixTimeDb [ !duplicated( subWorstRadixTimeDb$Threads.Number), ]
                            subWorstRadixTimeDb$compactNumbering <- 1:nrow(subWorstRadixTimeDb)
                            
                            worstRadixTime <- max( subWorstRadixTimeDb$Nanoseconds.per.Barrier)

                            # Plot Best and Worst Radix Chart
                            if ( (length( unique( factor( subBestRadixTimeDb$Radix))) != 1) ||
                                 (length( unique( factor( subWorstRadixTimeDb$Radix))) != 1) )
                            {
                                subBestRadixTimePDb <-subBestRadixTimeDb
                                subBestRadixTimePDb$Nanoseconds.per.Barrier <- NULL
                                subBestRadixTimePDb <- merge( subBestRadixTimePDb, subDb5)
                                
                                cfgP <- paste( paste( "Best Radix", titleEntity, sep = " "),
                                               cfg,
                                               sep = "\n")
                                plotFunc( subBestRadixTimePDb$Nanoseconds.per.Barrier ~ subBestRadixTimePDb$Threads.Number,
                                          ccol = 8,
                                          pch = 1,
                                          xlab = "Number of Threads",
                                          ylab = yAxisName,
                                          n.label = FALSE,
                                          ylim = c( 0, 1.05 * worstRadixTime),
                                          main = cfgP)
                                text( subBestRadixTimeDb$compactNumbering + threadAnnAdj, 
                                      subBestRadixTimeDb$Nanoseconds.per.Barrier, 
                                      subBestRadixTimeDb$Radix, 
                                      pos = 3)
                                points( subBestTimeDb$compactNumbering, subBestTimeDb$Nanoseconds.per.Barrier, col="green", pch = 2)
                                points( subWorstTimeDb$compactNumbering, subWorstTimeDb$Nanoseconds.per.Barrier, col="red", pch = 6)


                                subWorstRadixTimePDb <- subWorstRadixTimeDb
                                subWorstRadixTimePDb$Nanoseconds.per.Barrier <- NULL
                                subWorstRadixTimePDb <- merge( subWorstRadixTimePDb, subDb5)

                                cfgP <- paste( paste( "Worst Radix", titleEntity, sep = " "),
                                               cfg,
                                               sep = "\n")
                                plotFunc( subWorstRadixTimePDb$Nanoseconds.per.Barrier ~ subWorstRadixTimePDb$Threads.Number,
                                          ccol = 8,
                                          pch = 1,
                                          xlab = "Number of Threads",
                                          ylab = yAxisName,
                                          n.label = FALSE,
                                          ylim = c( 0, 1.05 * worstRadixTime),
                                          main = cfgP)
                                text( subWorstRadixTimeDb$compactNumbering + threadAnnAdj, 
                                      subWorstRadixTimeDb$Nanoseconds.per.Barrier, 
                                      subWorstRadixTimeDb$Radix, 
                                      pos = 3)
                                points( subBestTimeDb$compactNumbering, subBestTimeDb$Nanoseconds.per.Barrier, col="green", pch = 2)
                                points( subWorstTimeDb$compactNumbering, subWorstTimeDb$Nanoseconds.per.Barrier, col="red", pch = 6)
                            }

                            levelsL <- as.integer( levels( factor( subDb5$Radix)))
                            levelsL <- sort( levelsL)
                            subDb5$Radix <- factor( subDb5$Radix, levels=levelsL)
                            radixF <- factor( subDb5$Radix)
                            uniqueRadixV <- sort( unique( radixF))
                            for ( i6 in 1:length( uniqueRadixV))
                            {
                                # Plot Chart For Each Radix
                                radix <- uniqueRadixV [ i6 ]
                                subDb6 <- subDb5 [ subDb5$Radix == radix, ]
                                if ( nrow( subDb6) > 0 )
                                { 
                                    if ( interpolateRadix )
                                    {
                                        radixN <- as.numeric( levels( radix)[ as.integer( radix)]) 
                                        if ( radixN > 2 )
                                        {
                                            addMeanDb <- subMeanTimeDb [ (subMeanTimeDb$Barrier == barrier), ]
                                            addMeanDb <- addMeanDb [ (addMeanDb$Spinning == spinning), ]
                                            addMeanDb <- addMeanDb [ addMeanDb$Radix == (radixN - 1),]
                                            addMeanDb <- addMeanDb [ addMeanDb$Threads.Number < radixN,]
                                            addMeanDb$Radix <- radix
                                            subMeanTimeDb <- rbind( subMeanTimeDb, addMeanDb)

                                            addSubDb <- subDb5 [ subDb5$Radix == (radixN - 1),]
                                            addSubDb <- addSubDb [ addSubDb$Threads.Number < radixN,]
                                            addSubDb$Radix <- radix
                                            subDb6 <- rbind( subDb6, addSubDb)
                                            subDb5 <- rbind( subDb5, addSubDb)
                                            subDb3 <- rbind( subDb3, addSubDb)
                                        }
                                    }

                                    cfgR <- paste( paste( "radix", radix, sep = " = "), cfg, sep = "\n")
                                    plotFunc( subDb6$Nanoseconds.per.Barrier ~ subDb6$Threads.Number,
                                              ccol = 8,
                                              pch = 1,
                                              xlab = "Number of Threads",
                                              ylab = yAxisName,
                                              ylim = c( 0, 1.05 * worstRadixTime),
                                              n.label = FALSE,
                                              main = cfgR)
                                    points( subBestTimeDb$compactNumbering, 
                                            subBestTimeDb$Nanoseconds.per.Barrier, 
                                            col="green",
                                            pch = 2)
                                    points( subWorstTimeDb$compactNumbering,
                                            subWorstTimeDb$Nanoseconds.per.Barrier,
                                            col="red",
                                            pch = 6)
                                }
                            }

                            dev.off( )
                        }
                    }
                }

                if ( nrow( subDb3) > 0 )
                {
                    # Plot Best and Worst Barrier Charts
                    cfg <- paste( hostname, benchmark, experiment, sep = cfgSeparator)
                    pdf( paste( hostChartDir, paste( "best","worst", paste( cfg, "pdf", sep = "."), sep = cfgSeparator), sep = pathSeparator))

                    cfgFile <- paste( paste( hostname, arch, experiment, sep = cfgSeparator),".cfg", sep = "")
                    cfgFile <- paste( dbDir, cfgFile, sep = pathSeparator)
                    if ( file.exists( cfgFile) )
                    {
                        cfgInfo <- read.table( cfgFile, sep = "\n")
                        infoLines <- nrow( cfgInfo)
                        plot( 0:infoLines, type = "n", xaxt="n", yaxt="n", bty="n", xlab = "", ylab = "")
                        for ( iLine in 1:infoLines )
                        {
                            text( 4, infoLines - iLine, cfgInfo [ iLine, 1], adj = c( 0, NA))
                        }
                    }
                    {
                        curTLB <- 1
                        tL <- length( subBestTimeDb$Threads.Number)
                        maxN <- max( subBestTimePDb$Nanoseconds.per.Barrier)
                        while ( curTLB <= tL )
                        {
                            curTHB <- min( curTLB + plotStep - 1, tL)

                            cfgP <- paste( paste( "Best", titleEntity, sep = " "),
                                           cfg,
                                           sep = "\n")
                            subSubBestTimePDb <- subBestTimePDb [ subBestTimeDb[ curTLB, "Threads.Number"] <= subBestTimePDb$Threads.Number, ]
                            subSubBestTimePDb <- subSubBestTimePDb [ subSubBestTimePDb$Threads.Number <= subBestTimeDb[ curTHB, "Threads.Number"], ]
                            plotFunc( subSubBestTimePDb$Nanoseconds.per.Barrier ~ subSubBestTimePDb$Threads.Number,
                                      ccol=8,
                                      pch=1,
                                      cex=0.5,
                                      xlab="Number of Threads",
                                      ylab=yAxisName,
                                      n.label=FALSE,
                                      ylim = c( 0, maxN),
                                      main=cfgP)
                            annotateBestWorstValuesFunc( subBestTimeDb, curTLB, curTHB)
                            
                            curTLB <- curTLB + plotStep
                        }
                        curTLB <- 1
                        tL <- length( subWorstTimeDb$Threads.Number)
                        maxN <- max( subWorstTimePDb$Nanoseconds.per.Barrier)
                        while ( curTLB <= tL )
                        {
                            curTHB <- min( curTLB + plotStep - 1, tL)

                            cfgP <- paste(  paste( "Worst", titleEntity, sep = " "),
                                            cfg, 
                                            sep = "\n")
                            subSubWorstTimePDb <- subWorstTimePDb [ subWorstTimeDb[ curTLB, "Threads.Number"] <= subWorstTimePDb$Threads.Number, ]
                            subSubWorstTimePDb <- subSubWorstTimePDb [ subSubWorstTimePDb$Threads.Number <= subWorstTimeDb[ curTHB, "Threads.Number"], ]
                            plotFunc( subSubWorstTimePDb$Nanoseconds.per.Barrier ~ subSubWorstTimePDb$Threads.Number,
                                      ccol=8,
                                      pch=1,
                                      cex=0.5,
                                      xlab="Number of Threads",
                                      ylab=yAxisName,
                                      n.label=FALSE,
                                      ylim = c( 0, maxN),
                                      main=cfgP)
                            annotateBestWorstValuesFunc( subWorstTimeDb, curTLB, curTHB)
                            
                            curTLB <- curTLB + plotStep

                        }
                    }
                    {
                        subGeomeanThreadTimeDb <- aggregate( Nanoseconds.per.Barrier ~ 
                                                             Barrier + 
                                                             Radix + 
                                                             Spinning, 
                                                             data=subMeanTimeDb,
                                                             geomeanFunc)
                        subGeomeanThreadTimeDb <- subGeomeanThreadTimeDb [ order( subGeomeanThreadTimeDb$Nanoseconds.per.Barrier), ]

                        bestGeomeanThreadTime <- geomeanFunc( subBestTimeDb$Nanoseconds.per.Barrier)
                        worstGeomeanThreadTime <- geomeanFunc( subWorstTimeDb$Nanoseconds.per.Barrier)
                        namesArgs <- paste( subGeomeanThreadTimeDb$Barrier, 
                                            subGeomeanThreadTimeDb$Spinning,
                                            subGeomeanThreadTimeDb$Radix,
                                            sep = cfgSeparator)
                        barStep <- ceiling( length( namesArgs) / ceiling( length( namesArgs) / barStepMax))
                        curLB <- 1
                        while ( curLB <= length( namesArgs) )
                        {
                            curHB <- min( curLB + barStep - 1, length( namesArgs))
                            cfgP <- paste( cfg, "Geomean by Number Of Threads", sep = "\n")
                            barplot( subGeomeanThreadTimeDb$Nanoseconds.per.Barrier [ curLB : curHB ], 
                                     names.arg = namesArgs [ curLB : curHB ],
                                     las = 3,
                                     cex.names = min( 30 / barStep, 30 / barStepMin),
                                     ylab=yAxisName,
                                     main = cfgP,
                                     space = 0.1,
                                     width = 0.9,
                                     ylim = c( 0, worstGeomeanThreadTime),
                                     xlim = c( 1, barStep))
                            abline( b = 0, a = bestGeomeanThreadTime, col = "green", pch = 2)
                            abline( b = 0, a = worstGeomeanThreadTime, col = "red", pch = 6)
                            
                            curLB <- curLB + barStep 
                        }
                    }
                    {
                        threadsF <- factor( subMeanTimeDb$Threads.Number)
                        uniqueThreadsV <- sort( unique( threadsF))
                        for ( i7 in 1:length( uniqueThreadsV))
                        {
                            threads <- uniqueThreadsV [ i7 ]
                            subMeanTimeForThreadsDb <- subMeanTimeDb [ subMeanTimeDb$Threads.Number == threads, ]
                            subMeanTimeForThreadsDb <- subMeanTimeForThreadsDb [ order( subMeanTimeForThreadsDb$Nanoseconds.per.Barrier), ]
                            namesArgs <- paste( subMeanTimeForThreadsDb$Barrier, 
                                                subMeanTimeForThreadsDb$Spinning,
                                                subMeanTimeForThreadsDb$Radix,
                                                sep = cfgSeparator)
                            subMeanTimeForThreadsDb$namesArgs <- namesArgs
                            barStep <- ceiling( length( namesArgs) / ceiling( length( namesArgs) / barStepMax))
                            curLB <- 1
                            maxGM <- max( subMeanTimeForThreadsDb$Nanoseconds.per.Barrier)
                            while ( curLB <= length( namesArgs) )
                            {
                                curHB <- min( curLB + barStep - 1, length( namesArgs))
                                subSubMeanTimeForThreadsDb <- subMeanTimeForThreadsDb [ curLB : curHB, ]
                                subSubMeanTimeForThreadsDb$namesArgs <- 
                                    factor( subSubMeanTimeForThreadsDb$namesArgs, levels=subSubMeanTimeForThreadsDb$namesArgs)
                                subSubMeanTimeForThreadsDb$Nanoseconds.per.Barrier <- NULL
                                subSubMeanTimeForThreadsDb <- merge( subSubMeanTimeForThreadsDb, subDb3)
                                cfgP <- paste( paste( "Barriers", titleEntity, sep = " "),
                                               paste( "Number Of Threads = ", threads, sep = ""),
                                               cfg, 
                                               sep = "\n")
                                plotFunc( subSubMeanTimeForThreadsDb$Nanoseconds.per.Barrier ~
                                          subSubMeanTimeForThreadsDb$namesArgs, 
                                          las = 3,
                                          cex.axis = min( 30 / barStep, 30 / barStepMin),
                                          main = cfgP,
                                          ccol=8,
                                          cex=max( 24/barStep, 0.5), 
                                          pch=1,
                                          yaxt="n",
                                          ylab=yAxisName,
                                          ylim = c( 0, maxGM))
                                axis( 2)
                                curLB <- curLB + barStep 
                            }
                            subMeanTimeForThreadsDb$namesArgs <- NULL
                        }
                    }
                    dev.off( )
                }
            }
        }
    }
}
